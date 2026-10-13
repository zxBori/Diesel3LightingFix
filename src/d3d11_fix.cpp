#include "d3d11_fix.h"

#include <MinHook.h>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <superblt_flat.h>

// ------------------------------------------------------------
// Function types
// ------------------------------------------------------------

using SetResourceMinLODFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, FLOAT);

using PSSetShaderResourcesFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT,
                                                        ID3D11ShaderResourceView* const*);

// ------------------------------------------------------------
// Original functions
// ------------------------------------------------------------

static SetResourceMinLODFn g_originalSetResourceMinLOD = nullptr;
static PSSetShaderResourcesFn g_originalPSSetShaderResources = nullptr;

// ------------------------------------------------------------
// Check whether this is one of the suspicious tiny textures
// ------------------------------------------------------------

static bool IsProblemTexture(ID3D11Resource* resource)
{
	if (!resource)
		return false;

	ID3D11Texture2D* texture2D = nullptr;

	HRESULT hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture2D));

	if (FAILED(hr) || !texture2D)
		return false;

	D3D11_TEXTURE2D_DESC desc{};
	texture2D->GetDesc(&desc);

	const bool sizeMatch = desc.Width == 4 && desc.Height == 4 && desc.MipLevels == 2;

	const bool formatMatch = desc.Format == DXGI_FORMAT_BC1_TYPELESS || desc.Format == DXGI_FORMAT_BC3_TYPELESS;

	const bool clampMatch = (desc.MiscFlags & D3D11_RESOURCE_MISC_RESOURCE_CLAMP) != 0;

	texture2D->Release();

	return sizeMatch && formatMatch && clampMatch;
}

// ------------------------------------------------------------
// Hook: SetResourceMinLOD
//
// Prevent newly created problem textures from receiving
// an inaccessible MinLOD.
// ------------------------------------------------------------

static void STDMETHODCALLTYPE HookedSetResourceMinLOD(ID3D11DeviceContext* context, ID3D11Resource* resource,
                                                      FLOAT minLOD)
{
	FLOAT newMinLOD = minLOD;

	if (resource && minLOD >= 2.0f && IsProblemTexture(resource))
	{
		newMinLOD = 0.0f;
	}

	g_originalSetResourceMinLOD(context, resource, newMinLOD);
}

// ------------------------------------------------------------
// Hook: PSSetShaderResources
//
// Repair resources that already had the bad MinLOD state
// before this plugin was loaded.
// ------------------------------------------------------------

static void STDMETHODCALLTYPE HookedPSSetShaderResources(ID3D11DeviceContext* context, UINT startSlot, UINT numViews,
                                                         ID3D11ShaderResourceView* const* shaderResourceViews)
{
	if (context && shaderResourceViews)
	{
		for (UINT i = 0; i < numViews; ++i)
		{
			ID3D11ShaderResourceView* srv = shaderResourceViews[i];

			if (!srv)
				continue;

			ID3D11Resource* resource = nullptr;
			srv->GetResource(&resource);

			if (!resource)
				continue;

			if (IsProblemTexture(resource))
			{
				FLOAT currentLOD = context->GetResourceMinLOD(resource);

				if (currentLOD >= 2.0f)
				{
					context->SetResourceMinLOD(resource, 0.0f);
				}
			}

			resource->Release();
		}
	}

	g_originalPSSetShaderResources(context, startSlot, numViews, shaderResourceViews);
}

// ------------------------------------------------------------
// Install hooks
// ------------------------------------------------------------

bool InstallD3D11LightingFix()
{
	MH_STATUS status = MH_Initialize();

	if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: MH_Initialize failed");

		return false;
	}

	// --------------------------------------------------------
	// Create temporary D3D11 device/context
	// --------------------------------------------------------

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	D3D_FEATURE_LEVEL featureLevel{};

	const D3D_FEATURE_LEVEL requestedLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
	                                             D3D_FEATURE_LEVEL_10_0};

	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, requestedLevels,
	                               ARRAYSIZE(requestedLevels), D3D11_SDK_VERSION, &device, &featureLevel, &context);

	// Fallback for runtimes that reject 11_1
	if (hr == E_INVALIDARG)
	{
		const D3D_FEATURE_LEVEL fallbackLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
		                                            D3D_FEATURE_LEVEL_10_0};

		hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, fallbackLevels, ARRAYSIZE(fallbackLevels),
		                       D3D11_SDK_VERSION, &device, &featureLevel, &context);
	}

	if (FAILED(hr) || !device || !context)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: D3D11CreateDevice failed");

		if (context)
			context->Release();

		if (device)
			device->Release();

		return false;
	}

	// --------------------------------------------------------
	// Get ID3D11DeviceContext vtable
	// --------------------------------------------------------

	void** vtable = *reinterpret_cast<void***>(context);

	// ID3D11DeviceContext:
	// index 8  = PSSetShaderResources
	// index 55 = SetResourceMinLOD

	void* psSetShaderResourcesTarget = vtable[8];

	void* setResourceMinLODTarget = vtable[55];

	if (!psSetShaderResourcesTarget || !setResourceMinLODTarget)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: D3D11 target function is null");

		context->Release();
		device->Release();

		return false;
	}

	// --------------------------------------------------------
	// Hook SetResourceMinLOD
	// --------------------------------------------------------

	status = MH_CreateHook(setResourceMinLODTarget, &HookedSetResourceMinLOD,
	                       reinterpret_cast<void**>(&g_originalSetResourceMinLOD));

	if (status != MH_OK)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: SetResourceMinLOD MH_CreateHook failed");

		context->Release();
		device->Release();

		return false;
	}

	status = MH_EnableHook(setResourceMinLODTarget);

	if (status != MH_OK)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: SetResourceMinLOD MH_EnableHook failed");

		context->Release();
		device->Release();

		return false;
	}

	// --------------------------------------------------------
	// Hook PSSetShaderResources
	// --------------------------------------------------------

	status = MH_CreateHook(psSetShaderResourcesTarget, &HookedPSSetShaderResources,
	                       reinterpret_cast<void**>(&g_originalPSSetShaderResources));

	if (status != MH_OK)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: PSSetShaderResources MH_CreateHook failed");

		context->Release();
		device->Release();

		return false;
	}

	status = MH_EnableHook(psSetShaderResourcesTarget);

	if (status != MH_OK)
	{
		PD2HOOK_LOG_ERROR("Diesel3LightingFix: PSSetShaderResources MH_EnableHook failed");

		context->Release();
		device->Release();

		return false;
	}

	context->Release();
	device->Release();

	PD2HOOK_LOG_LOG("Diesel3LightingFix: hooks installed");

	return true;
}