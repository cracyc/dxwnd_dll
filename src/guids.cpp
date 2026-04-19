#include <windows.h>
#include <stdio.h>
#include <ddraw.h>

// to search new ones, try this: https://www.magnumdb.com/search

char *ExplainGUID(GUID FAR *lpguid)
{
	static char *sguid;
	switch ((DWORD)lpguid){
		case NULL:						return "NULL"; break;
		case DDCREATE_HARDWAREONLY:		return "DDCREATE_HARDWAREONLY"; break;
		case DDCREATE_EMULATIONONLY:	return "DDCREATE_EMULATIONONLY"; break;
	}
	switch (lpguid->Data1){

		// COM

		case 0x00000000: sguid="IID_IUnknown"; break; // "00000000-0000-0000-C000-000000000046"
		case 0x00000001: sguid="IID_IClassFactory"; break; // "00000001-0000-0000-C000-000000000046"
		case 0x00000002: sguid="IID_IMalloc"; break; // "00000002-0000-0000-C000-000000000046"
		case 0x00020400: sguid="IID_IDispatch"; break; // "00020400-0000-0000-C000-000000000046"
		case 0x0000010b: sguid="IID_IPersistFile"; break; // "0000010b-0000-0000-C000-000000000046"
		case 0x0000010c: sguid="IID_IPersist"; break; // "0000010c-0000-0000-C000-000000000046"
		case 0x0000010d: sguid="IID_IViewObject"; break; // "0000010d-0000-0000-C000-000000000046"
		case 0x00000127: sguid="IID_IViewObject2"; break; // "00000127-0000-0000-C000-000000000046"
		case 0x0000013D: sguid="IID_IClientSecurity"; break; // "0000013D-0000-0000-C000-000000000046"
		case 0x0000013E: sguid="IID_IServerSecurity"; break; // "0000013E-0000-0000-C000-000000000046"
		case 0x00000140: sguid="IID_IClassActivator"; break; // "00000140-0000-0000-C000-000000000046"
		case 0x0000002F: sguid="IID_RecordInfo"; break;
		case 0x00020403: sguid="IID_ITypeComp"; break;
		case 0x00020401: sguid="IID_ITypeInfo"; break;
		case 0x00020412: sguid="IID_ITypeInfo2"; break;
		case 0x00020402: sguid="IID_ITypeLib"; break;
		case 0x00020404: sguid="IID_IEnumVARIANT"; break;
		case 0x1f486a52: sguid="CLSID_MemoryMappedCacheMgr"; break;
		case 0xECF31D61: sguid="IID_IMemoryMappedCacheMgr"; break;
		case 0x725F645B: sguid="CLSID_FileOpenDialogLegacy"; break;
		case 0x00000109: sguid="IPersistStream"; break; // "00000109-0000-0000-C000-000000000046"

		// Shell32

		case 0x00021401: sguid="CLSID_ShellLink"; break;
		case 0x000214EE: sguid="IID_IShellLinkA"; break;
		case 0x000214F9: sguid="IID_IShellLinkW"; break;
		case 0x000214E1: sguid="IID_INewShortcutHookA"; break;
		case 0x000214E4: sguid="IID_IContextMenu"; break;
		case 0x000214E5: sguid="IID_IShellIcon"; break;

		// DirectInput
		// thanks to https://www.betaarchive.com/forum/viewtopic.php?t=42019 DirectX archive
		case 0x25E609E0: sguid="CLSID_DirectInput"; break; // 25E609E0-B259-11CF-BFC7-444553540000
		case 0x25E609E1: sguid="CLSID_DirectInputDevice"; break; //25E609E1-B259-11CF-BFC7-444553540000
		case 0xA36D02E0: sguid="GUID_XAxis"; break; // A36D02E0-C9F3-11CF-BFC7-444553540000
		case 0xA36D02E1: sguid="GUID_YAxis"; break; // A36D02E1-C9F3-11CF-BFC7-444553540000
		case 0xA36D02E2: sguid="GUID_ZAxis"; break; // A36D02E2-C9F3-11CF-BFC7-444553540000
		case 0xA36D02E3: sguid="GUID_RAxis"; break; // A36D02E3-C9F3-11CF-BFC7-444553540000
		case 0xA36D02E4: sguid="GUID_UAxis"; break; // A36D02E4-C9F3-11CF-BFC7-444553540000
		case 0xA36D02E5: sguid="GUID_VAxis"; break; // A36D02E5-C9F3-11CF-BFC7-444553540000
		case 0xA36D02F0: sguid="GUID_Button"; break; // A36D02F0-C9F3-11CF-BFC7-444553540000
		case 0x55728220: sguid="GUID_Key"; break; // 55728220-D33C-11CF-BFC7-444553540000
		case 0xA36D02F2: sguid="GUID_POV"; break; // A36D02F2-C9F3-11CF-BFC7-444553540000
		case 0xA36D02F3: sguid="GUID_Unknown"; break; // A36D02F3-C9F3-11CF-BFC7-444553540000
		case 0x6F1D2B60: sguid="GUID_SysMouse"; break; // 6F1D2B60-D5A0-11CF-BFC7-444553540000
		case 0x6F1D2B61: sguid="GUID_SysKeyboard"; break; // 6F1D2B61-D5A0-11CF-BFC7-444553540000
		case 0x6F1D2B70: sguid="GUID_Joystick"; break; // 6F1D2B70-D5A0-11CF-BFC7-444553540000
		case 0x6F1D2B80: sguid="GUID_SysMouseEm"; break; // 6F1D2B80-D5A0-11CF-BFC7-444553540000
		case 0x6F1D2B81: sguid="GUID_SysMouseEm2"; break; // 6F1D2B81-D5A0-11CF-BFC7-444553540000
		case 0x6F1D2B82: sguid="GUID_SysKeyboardEm"; break; // 6F1D2B82-D5A0-11CF-BFC7-444553540000
		case 0x6F1D2B83: sguid="GUID_SysKeyboardEm2"; break; // 6F1D2B83-D5A0-11CF-BFC7-444553540000
		case 0x89521360: sguid="IID_IDirectInputA"; break;
		case 0x89521361: sguid="IID_IDirectInputW"; break;
		case 0x5944E662: sguid="IID_IDirectInput2A"; break;
		case 0x5944E663: sguid="IID_IDirectInput2W"; break;
		case 0x9A4CB684: sguid="IID_IDirectInput7A"; break;
		case 0x9A4CB685: sguid="IID_IDirectInput7W"; break;
		case 0xBF798030: sguid="IID_IDirectInput8A"; break;
		case 0xBF798031: sguid="IID_IDirectInput8W"; break;
		case 0x5944E680: sguid="IID_IDirectInputDeviceA"; break;
		case 0x5944E681: sguid="IID_IDirectInputDeviceW"; break;
		case 0x5944E682: sguid="IID_IDirectInputDevice2A"; break;
		case 0x5944E683: sguid="IID_IDirectInputDevice2W"; break;
		case 0x57D7C6BC: sguid="IID_IDirectInputDevice7A"; break;
		case 0x57D7C6BD: sguid="IID_IDirectInputDevice7W"; break;
		case 0x54D41080: sguid="IID_IDirectInputDevice8A"; break;
		case 0x54D41081: sguid="IID_IDirectInputDevice8W"; break;
		case 0xE7E1F7C0: sguid="IID_IDirectInputEffect"; break;

		// DirectDraw

		case 0x6C14DB80: sguid="IID_IDirectDraw"; break;
		case 0xB3A6F3E0: sguid="IID_IDirectDraw2"; break;
		case 0x618f8ad4: sguid="IID_IDirectDraw3"; break;
		case 0x9c59509a: sguid="IID_IDirectDraw4"; break;
		case 0x15e65ec0: sguid="IID_IDirectDraw7"; break;
		case 0x6C14DB81: sguid="IID_IDirectDrawSurface"; break;
		case 0x57805885: sguid="IID_IDirectDrawSurface2"; break;
		case 0xDA044E00: sguid="IID_IDirectDrawSurface3"; break;
		case 0x0B2B8630: sguid="IID_IDirectDrawSurface4"; break;
		case 0x06675a80: sguid="IID_IDirectDrawSurface7"; break;
		case 0x6C14DB84: sguid="IID_IDirectDrawPalette"; break;
		case 0x6C14DB85: sguid="IID_IDirectDrawClipper"; break;
		case 0x4B9F0EE0: sguid="IID_IDirectDrawColorControl"; break;
		case 0x69C11C3E: sguid="IID_IDirectDrawGammaControl"; break;
		case 0xA4665C60: sguid="IID_IDirect3DRGBDevice"; break;
		case 0x84E63dE0: sguid="IID_IDirect3DHALDevice"; break;
		case 0x3BBA0080: sguid="IID_IDirect3D"; break;
		case 0x6aae1ec1: sguid="IID_IDirect3D2"; break;
		case 0xbb223240: sguid="IID_IDirect3D3"; break;
		case 0xf5049e77: sguid="IID_IDirect3D7"; break;
		case 0x64108800: sguid="IID_IDirect3DDevice"; break;
		case 0x93281501: sguid="IID_IDirect3DDevice2"; break;
		case 0xb0ab3b60: sguid="IID_IDirect3DDevice3"; break;
		case 0xf5049e79: sguid="IID_IDirect3DDevice7"; break;
		case 0x2CDCD9E0: sguid="IID_IDirect3DTexture"; break;
		case 0x93281502: sguid="IID_IDirect3DTexture2"; break;
		case 0x4417C142: sguid="IID_IDirect3DLight"; break;
		case 0x4417C144: sguid="IID_IDirect3DMaterial"; break;
		case 0x93281503: sguid="IID_IDirect3DMaterial2"; break;
		case 0xca9c46f4: sguid="IID_IDirect3DMaterial3"; break;
		case 0x4417C145: sguid="IID_IDirect3DExecuteBuffer"; break;
		case 0x4417C146: sguid="IID_IDirect3DViewport"; break;
		case 0x93281500: sguid="IID_IDirect3DViewport2"; break;
		case 0xb0ab3b61: sguid="IID_IDirect3DViewport3"; break;
		case 0x7a503555: sguid="IID_IDirect3DVertexBuffer"; break;
		case 0xf5049e7d: sguid="IID_IDirect3DVertexBuffer7"; break;
		case 0xF2086B20: sguid="IID_IDirect3DRampDevice"; break;
		case 0x881949a1: sguid="IID_IDirect3DMMXDevice"; break;
		case 0x50936643: sguid="IID_IDirect3DRefDevice"; break;
		case 0x8767df22: sguid="IID_IDirect3DNullDevice"; break;
		case 0xf5049e78: sguid="IID_IDirect3DTnLHalDevice"; break;
		case 0xD7B70EE0: sguid="CLSID_DirectDraw"; break;
		case 0x3c305196: sguid="CLSID_DirectDraw7"; break;
		case 0x593817A0: sguid="CLSID_DirectDrawClipper"; break;

		// Direct3D8

		case 0x1dd9e8da: sguid="IID_IDirect3D8"; break; // 0x1dd9e8da, 0x1c77, 0x4d40, 0xb0, 0xcf, 0x98, 0xfe, 0xfd, 0xff, 0x95, 0x12
		case 0x7385e5df: sguid="IID_IDirect3DDevice8"; break; // 0x7385e5df, 0x8fe8, 0x41d5, 0x86, 0xb6, 0xd7, 0xb4, 0x85, 0x47, 0xb6, 0xcf
		case 0x1b36bb7b: sguid="IID_IDirect3DResource8"; break; // 0x1b36bb7b, 0x9b7, 0x410a, 0xb4, 0x45, 0x7d, 0x14, 0x30, 0xd7, 0xb3, 0x3f
		case 0xb4211cfa: sguid="IID_IDirect3DBaseTexture8"; break; // 0xb4211cfa, 0x51b9, 0x4a9f, 0xab, 0x78, 0xdb, 0x99, 0xb2, 0xbb, 0x67, 0x8e
		case 0xe4cdd575: sguid="IID_IDirect3DTexture8"; break; // 0xe4cdd575, 0x2866, 0x4f01, 0xb1, 0x2e, 0x7e, 0xec, 0xe1, 0xec, 0x93, 0x58
		case 0x3ee5b968: sguid="IID_IDirect3DCubeTexture8"; break; // 0x3ee5b968, 0x2aca, 0x4c34, 0x8b, 0xb5, 0x7e, 0x0c, 0x3d, 0x19, 0xb7, 0x50
		case 0x4b8aaafa: sguid="IID_IDirect3DVolumeTexture8"; break; // 0x4b8aaafa, 0x140f, 0x42ba, 0x91, 0x31, 0x59, 0x7e, 0xaf, 0xaa, 0x2e, 0xad
		case 0x8aeeeac7: sguid="IID_IDirect3DVertexBuffer8"; break; // 0x8aeeeac7, 0x05f9, 0x44d4, 0xb5, 0x91, 0x00, 0x0b, 0x0d, 0xf1, 0xcb, 0x95
		case 0x0e689c9a: sguid="IID_IDirect3DIndexBuffer8"; break; // 0x0e689c9a, 0x053d, 0x44a0, 0x9d, 0x92, 0xdb, 0x0e, 0x3d, 0x75, 0x0f, 0x86
		case 0xb96eebca: sguid="IID_IDirect3DSurface8"; break; // 0xb96eebca, 0xb326, 0x4ea5, 0x88, 0x2f, 0x2f, 0xf5, 0xba, 0xe0, 0x21, 0xdd
		case 0xbd7349f5: sguid="IID_IDirect3DVolume8"; break; // 0xbd7349f5, 0x14f1, 0x42e4, 0x9c, 0x79, 0x97, 0x23, 0x80, 0xdb, 0x40, 0xc0
		case 0x928c088b: sguid="IID_IDirect3DSwapChain8"; break; // 0x928c088b, 0x76b9, 0x4c6b, 0xa5, 0x36, 0xa5, 0x90, 0x85, 0x38, 0x76, 0xcd

		// Direct3D9

		case 0x81bdcbca: sguid="IID_IDirect3D9"; break;
		case 0x02177241: sguid="IID_IDirect3D9Ex"; break;
		case 0xd0223b96: sguid="IID_IDirect3DDevice9"; break;
		case 0xb18b10ce: sguid="IID_IDirect3DDevice9Ex"; break;
		case 0x580ca87e: sguid="IID_IDirect3DBaseTexture9"; break;
		case 0x85c31227: sguid="IID_IDirect3DTexture9"; break;
		case 0x0cfbaf3a: sguid="IID_IDirect3DSurface9"; break;
		case 0x1bab8e96: sguid="IID_IDirectDrawSurfaceNew"; break;

		case 0x05eec05d: sguid="IID_IDirect3DResource9"; break;
		case 0xb64bb1b5: sguid="IID_IDirect3DVertexBuffer9"; break;
		case 0x24f416e6: sguid="IID_IDirect3DVolume9"; break;
		case 0x794950f2: sguid="IID_IDirect3DSwapChain9"; break;
		case 0x91886caf: sguid="IID_IDirect3DSwapChain9Ex"; break;
		case 0x7c9dd65e: sguid="IID_IDirect3DIndexBuffer9"; break;
		case 0xfff32f81: sguid="IID_IDirect3DCubeTexture9"; break;
		case 0x2518526c: sguid="IID_IDirect3DVolumeTexture9"; break;
		case 0xdd13c59c: sguid="IID_IDirect3DVertexDeclaration9"; break;
		case 0xefc5557e: sguid="IID_IDirect3DVertexShader9"; break;
		case 0x6d3bdbdc: sguid="IID_IDirect3DPixelShader9"; break;
		case 0xb07c4fe5: sguid="IID_IDirect3DStateBlock9"; break;
		case 0xd9771460: sguid="IID_IDirect3DQuery9"; break;

		// D3D10

		case 0x9B7E4C00: sguid="ID3D10DeviceChild"; break;
		case 0x2B4B1CC8: sguid="ID3D10DepthStencilState"; break;
		case 0xEDAD8D19: sguid="ID3D10BlendState"; break;
		case 0xA2A07292: sguid="ID3D10RasterizerState"; break;
		case 0x9B7E4C01: sguid="ID3D10Resource"; break;
		case 0x9B7E4C02: sguid="ID3D10Buffer"; break;
		case 0x9B7E4C03: sguid="ID3D10Texture1D"; break;
		case 0x9B7E4C04: sguid="ID3D10Texture2D"; break;
		case 0x9B7E4C05: sguid="ID3D10Texture3D"; break;
		case 0xC902B03F: sguid="ID3D10View"; break;
		case 0x9B7E4C07: sguid="ID3D10ShaderResourceView"; break;
		case 0x9B7E4C08: sguid="ID3D10RenderTargetView"; break;
		case 0x9B7E4C09: sguid="ID3D10DepthStencilView"; break;
		case 0x9B7E4C0A: sguid="ID3D10VertexShader"; break;
		case 0x6316BE88: sguid="ID3D10GeometryShader"; break;
		case 0x4968B601: sguid="ID3D10PixelShader"; break;
		case 0x9B7E4C0B: sguid="ID3D10InputLayout"; break;
		case 0x9B7E4C0C: sguid="ID3D10SamplerState"; break;
		case 0x9B7E4C0D: sguid="ID3D10Asynchronous"; break;
		case 0x9B7E4C0E: sguid="ID3D10Query"; break;
		case 0x9B7E4C10: sguid="ID3D10Predicate"; break;
		case 0x9B7E4C11: sguid="ID3D10Counter"; break;
		case 0x9B7E4C0F: sguid="ID3D10Device"; break;
		case 0x9B7E4E00: sguid="ID3D10Multithread"; break;
		case 0x9B7E4C8F: sguid="ID3D10Device1"; break;
		case 0xEDAD8D99: sguid="ID3D10BlendState1"; break;
		case 0x9B7E4C87: sguid="ID3D10ShaderResourceView1"; break;

		// D3D11

		case 0x79CF2233: sguid="IID_ID3D11Debug"; break;
		case 0xa04bfb29: sguid="IID_ID3D11Device1"; break;
		case 0x9d06dffa: sguid="IID_ID3D11Device2"; break;
		case 0xc2931aea: sguid="IID_ID3D11VideoDecoderOutputView"; break; // "c2931aea-2a85-4f20-860f-fba1fd256e18"
		case 0x3c9c5b51: sguid="IID_ID3D11VideoDecoder"; break; // "3c9c5b51-995d-48d1-9b8d-fa5caeded65c"
		case 0x61f21c45: sguid="IID_ID3D11VideoContext"; break; // "61f21c45-3c0e-4a74-9cea-67100d9ad5e4"
		case 0x10ec4d5b: sguid="IID_ID3D11VideoDevice"; break; // "10ec4d5b-975a-4689-b9e4-d0aac30fe333"

		// D3D12

		case 0x189819f1: sguid="IID_ID3D12Device"; break; // 189819F1-1DB6-4B57-BE54-1821339B85F7 

		// Direct3DRM

		// found on https://github.com/nihilus/GUID-Finder/blob/master/GUID-Finder/Classes.txt
		case 0x4516EC41: sguid="CDirect3DRM"; break; // 4516EC41-8F20-11D0-9B6D-0000C0781BC3 CDirect3DRM
		case 0x5434E72D: sguid="CDirect3DRMClippedVisual"; break; // 5434E72D-6D66-11D1-BB0B-0000F875865A CDirect3DRMClippedVisual
		case 0x4516EC40: sguid="CDirect3DRMProgressiveMesh"; break; // 4516EC40-8F20-11D0-9B6D-0000C0781BC3 CDirect3DRMProgressiveMesh
		case 0x0DE9EAA8: sguid="CDirect3DRMTextureInterpolator"; break; // 0DE9EAA8-3B84-11D0-9B6D-0000C0781BC3 CDirect3DRMTextureInterpolator
		case 0x0DE9EAA7: sguid="CDirect3DRMMaterialInterpolator"; break; // 0DE9EAA7-3B84-11D0-9B6D-0000C0781BC3 CDirect3DRMMaterialInterpolator
		case 0x0DE9EAA6: sguid="CDirect3DRMLightInterpolator"; break; // 0DE9EAA6-3B84-11D0-9B6D-0000C0781BC3 CDirect3DRMLightInterpolator
		case 0x0DE9EAA3: sguid="CDirect3DRMMeshInterpolator"; break; // 0DE9EAA3-3B84-11D0-9B6D-0000C0781BC3 CDirect3DRMMeshInterpolator
		case 0x0DE9EAA2: sguid="CDirect3DRMFrameInterpolator"; break; // 0DE9EAA2-3B84-11D0-9B6D-0000C0781BC3 CDirect3DRMFrameInterpolator
		case 0x0DE9EAA1: sguid="CDirect3DRMViewportInterpolator"; break; // 0DE9EAA1-3B84-11D0-9B6D-0000C0781BC3 CDirect3DRMViewportInterpolator
		case 0x4FA3569B: sguid="CDirect3DRMShadow"; break; // 4FA3569B-623F-11CF-AC4A-0000C03825A1 CDirect3DRMShadow
		case 0x4FA3569A: sguid="CDirect3DRMUserVisual"; break; // 4FA3569A-623F-11CF-AC4A-0000C03825A1 CDirect3DRMUserVisual
		case 0x4FA35699: sguid="CDirect3DRMAnimationSet"; break; // 4FA35699-623F-11CF-AC4A-0000C03825A1 CDirect3DRMAnimationSet
		case 0x4FA35698: sguid="CDirect3DRMAnimation"; break; // 4FA35698-623F-11CF-AC4A-0000C03825A1 CDirect3DRMAnimation
		case 0x4FA35697: sguid="CDirect3DRMMaterial"; break; // 4FA35697-623F-11CF-AC4A-0000C03825A1 CDirect3DRMMaterial
		case 0x4FA35696: sguid="CDirect3DRMWrap"; break; // 4FA35696-623F-11CF-AC4A-0000C03825A1 CDirect3DRMWrap
		case 0x4FA35695: sguid="CDirect3DRMTexture"; break; // 4FA35695-623F-11CF-AC4A-0000C03825A1 CDirect3DRMTexture
		case 0x4FA35694: sguid="CDirect3DRMLight"; break; // 4FA35694-623F-11CF-AC4A-0000C03825A1 CDirect3DRMLight
		case 0x4FA35693: sguid="CDirect3DRMFace"; break; // 4FA35693-623F-11CF-AC4A-0000C03825A1 CDirect3DRMFace
		case 0x4FA35692: sguid="CDirect3DRMMeshBuilder"; break; // 4FA35692-623F-11CF-AC4A-0000C03825A1 CDirect3DRMMeshBuilder
		case 0x4FA35691: sguid="CDirect3DRMMesh"; break; // 4FA35691-623F-11CF-AC4A-0000C03825A1 CDirect3DRMMesh
		case 0x4FA35690: sguid="CDirect3DRMFrame"; break; // 4FA35690-623F-11CF-AC4A-0000C03825A1 CDirect3DRMFrame
		case 0x4FA3568F: sguid="CDirect3DRMViewport"; break; // 4FA3568F-623F-11CF-AC4A-0000C03825A1 CDirect3DRMViewport
		case 0x4FA3568E: sguid="CDirect3DRMDevice"; break; // 4FA3568E-623F-11CF-AC4A-0000C03825A1 CDirect3DRMDevice
		// from D3DRM.H
		case 0x2bc49361: sguid="IID_IDirect3DRM"; break; // 0x2bc49361, 0x8327, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
		case 0x4516ecc8: sguid="IID_IDirect3DRM2"; break; // 0x4516ecc8, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
		case 0x4516ec83: sguid="IID_IDirect3DRM3"; break; // 0x4516ec83, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
		case 0xc5016cc0: sguid="IID_IDirect3DRMWinDevice"; break; // 0xc5016cc0, 0xd273, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);

		// WMI
		case 0x4590f811: sguid="CLSID_WbemLocator"; break; // {4590f811-1d3a-11d0-891f-00aa004b2e24} 
		case 0xdc12a687: sguid="IID_IWbemLocator"; break; // {dc12a687-737f-11cf-884d-00aa004b2e24}

		// DirectSound

		case 0x47d4d946: sguid="CLSID_DirectSound"; break; // 0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0
		case 0x3901cc3f: sguid="CLSID_DirectSound8"; break; // 3901CC3F-84B5-4FA4-BA35-AA8172B8A09B
		case 0xb0210780: sguid="CLSID_DirectSoundCapture"; break;
		case 0xe4bcac13: sguid="CLSID_DirectSoundCapture8"; break;
		case 0xfea4300c: sguid="CLSID_DirectSoundFullDuplex"; break;
		case 0xca503b60: sguid="CLSID_EAXDirectSound8"; break; // 0xca503b60,0xb176,0x11d4,0xa0,0x94,0xd0,0xc0,0xbf,0x3a,0x56,0xc
		case 0x79376820: sguid="CLSID_DSoundRender"; break; // 79376820-07d0-11cf-a24d-0020afd79767

		case 0x279afa83: sguid="IID_IDirectSound"; break; // 0x279AFA83,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60
		case 0x279AFA85: sguid="IID_IDirectSoundBuffer"; break; // 0x279AFA85,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60
		case 0xB0210783: sguid="IID_IDirectSoundNotify"; break; // 0xB0210783,0x89cd,0x11d0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16
		case 0x279AFA84: sguid="IID_IDirectSound3DListener"; break; // 0x279AFA84,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60
		case 0x279AFA86: sguid="IID_IDirectSound3DBuffer"; break; // 0x279AFA86,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60
		case 0xB0210781: sguid="IID_IDirectSoundCapture"; break; // 0xB0210781,0x89CD,0x11D0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16
		case 0xB0210782: sguid="IID_IDirectSoundCaptureBuffer"; break; // 0xB0210782,0x89CD,0x11D0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16
		case 0x31EFAC30: sguid="IID_IKsPropertySet"; break; // 0x31EFAC30,0x515C,0x11D0,0xA9,0xAA,0x00,0xAA,0x00,0x61,0xBE,0x93
		case 0xdef00000: sguid="DSDEVID_DefaultPlayback"; break;
		case 0xdef00001: sguid="DSDEVID_DefaultCapture"; break;
		case 0xdef00002: sguid="DSDEVID_DefaultVoicePlayback"; break;
		case 0xdef00003: sguid="DSDEVID_DefaultVoiceCapture"; break;

		case 0xC50A7E93: sguid="IDirectSound8"; break; // C50A7E93-F395-4834-9EF6-7FA99DE50966
		case 0x6825a449: sguid="IDirectSoundBuffer8"; break; // 6825a449-7524-4d82-920f-50e36ab3ab1e
		case 0x00990df4: sguid="IDirectSoundCaptureBuffer8"; break; // 00990df4-0dbb-4872-833e-6d303e80aeb6

		case 0x56a86897: sguid="IID_IReferenceClock"; break; // 0x56a86897, 0x0ad4, 0x11ce, 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

			// DirectPlay

		case 0xD1EB6D20: sguid="CLSID_DirectPlay"; break; // "D1EB6D20-8923-11d0-9D97-00A0C90A43CB"  DirectPlay;
		case 0x2b74f7c0: sguid="IID_DirectPlay2"; break; // "2b74f7c0-9154-11cf-a9cd-00aa006886e3"  IDirectPlay2;
		case 0x9d460580: sguid="IID_DirectPlay2A"; break; // "9d460580-a822-11cf-960c-0080c7534e82"  IDirectPlay2A;
		case 0x133efe40: sguid="IID_DirectPlay3"; break; // "133efe40-32dc-11d0-9cfb-00a0c90a43cb"))  IDirectPlay3;
		case 0x133efe41: sguid="IID_DirectPlay3A"; break; // "133efe41-32dc-11d0-9cfb-00a0c90a43cb"))  IDirectPlay3A;
		case 0x0ab1c530: sguid="IID_DirectPlay4"; break; // "0ab1c530-4745-11d1-a7a1-0000f803abfc"))  IDirectPlay4;
		case 0x0ab1c531: sguid="IID_DirectPlay4A"; break; // "0ab1c531-4745-11d1-a7a1-0000f803abfc"))  IDirectPlay4A;
		case 0x685BC400: sguid="DPSPGUID_IPX"; break; // "685BC400-9D2C-11cf-A9CD-00AA006886E3")) DPSPGUID_IPX;
		case 0x36E95EE0: sguid="DPSPGUID_TCPIP"; break; // "36E95EE0-8577-11cf-960C-0080C7534E82")) DPSPGUID_TCPIP;
		case 0x0F1D6860: sguid="DPSPGUID_SERIAL"; break; // "0F1D6860-88D9-11cf-9C4E-00A0C905425E")) DPSPGUID_SERIAL;
		case 0x44EAA760: sguid="DPSPGUID_MODEM"; break; // "44EAA760-CB68-11cf-9C4E-00A0C905425E")) DPSPGUID_MODEM;

		// AMM stream

		case 0x49c47ce5: sguid="AMMultiMediaStream"; break; // 49c47ce5-9ba4-11d0-8212-00c04fc32c45
		case 0x49c47ce4: sguid="CLSID_AMDirectDrawStream"; break; // 49c47ce4-9ba4-11d0-8212-00c04fc32c45
		case 0x8496e040: sguid="CLSID_AMAudioStream"; break; // 8496e040-af4c-11d0-8212-00c04fc32c45
		case 0xf2468580: sguid="CLSID_AMAudioData"; break; // f2468580-af8a-11d0-8212-00c04fc32c45
		case 0xCF0F2F7C: sguid="CLSID_AMMediaTypeStream"; break; // CF0F2F7C-F7BF-11d0-900D-00C04FD9189D

		// mixed common ones ...

		case 0x4fd2a833: sguid="IDirectDrawFactory"; break; //4fd2a833-86c8-11d0-8fca-00c04fd9189d
		case 0x4fd2a832: sguid="DirectDrawEx_Object"; break; //4fd2a832-86c8-11d0-8fca-00c04fd9189d 
		case 0xA65B8071: sguid="CLSID_DxDiagProvider"; break; // A65B8071-3BFE-4213-9A5B-491DA4461CA7
		case 0xD8F1EEE0: sguid="CLSID_A3d"; break; // {D8F1EEE0-F634-11CF-8700-00A0245D918B}
		case 0xD8F1EEE1: sguid="IID_IA3d"; break; // {D8F1EEE1-F634-11CF-8700-00A0245D918B}
		case 0xFB80D1E0: sguid="IID_IA3d2"; break; // {FB80D1E0-98D3-11D1-90FB-006008A1F441}
		case 0xE4C40280: sguid="IID_IA3d4"; break; // {E4C40280-CCBA-11D2-9DCF-00500411582F}
		case 0x4315D437: sguid="IID_IDeviceMoniker"; break; // {4315D437-5B8C-11D0-BD3B-00A0C911CE86}
		case 0x9C6B4CB0: sguid="IID_IDxDiagProvider"; break; // {9C6B4CB0-23F8-49CC-A3ED-45A55000A6D2} dxdiag.h
		case 0x7D0F462F: sguid="IID_IDxDiagContainer"; break; // {0x7D0F462F-0x4064-0x4862-BC7F-933E5058C10F} dxdiag.h
		case 0xA38CC06E: sguid="IID_IImageSync"; break; // {A38CC06E-5926-48DF-9926-571458145E80} vmrp.h
		case 0x1DBCA562: sguid="IID_IImageSyncNotifyEvent"; break; // 1DBCA562-5C92-474a-A276-382079164970 vmrp.h
		case 0xA67F6A0D: sguid="IID_IImageSyncControl"; break; // A67F6A0D-883B-44ce-AA93-87BA3017E19C vmrp.h
		case 0x56949f22: sguid="IID_IVMRMixerControlInternal"; break; // 56949f22-aa07-4061-bb8c-10159d8f92e5 vmrp.h
		case 0x43062408: sguid="IID_IVMRMixerStream"; break; // 43062408-3d55-43cc-9415-0daf218db422 vmrp.h
		case 0xede80b5c: sguid="IID_IVMRFilterConfigInternal"; break; // ede80b5c-bad6-4623-b537-65 58 6c 9f 8d fd vmrp.h
		case 0x56a868c0: sguid="IID_IMediaEventEx"; break; //56a868c0-0ad4-11ce-b03a-0020af0ba770
		case 0x060af76c: sguid="CLSID_SeekingPassThru"; break; //060af76c-68dd-11d0-8fc1-00c04fd9189d

		// direct music

		case 0x636B9F10: sguid="CLSID_DirectMusic"; break; // 636B9F10-0C7D-11D1-95B2-0020AFDC7421
		case 0x6536115a: sguid="IDirectMusic"; break; // 6536115A-7B2D-11D2-BA18-0000F875AC12
		case 0xd2ac2892: sguid="CLSID_DirectMusicLoader"; break; // {d2ac2892-b39b-11d1-8704-00600893b1bd}
		case 0xD2AC2881: sguid="CLSID_DirectMusicPerformance"; break; // {D2AC2881-B39B-11D1-8704-00600893B1BD}
		case 0x2ffaaca2: sguid="IDirectMusicLoader"; break; // {2ffaaca2-5dca-11d2-afa6-00aa0024d8b6}
		case 0x19e7c08c: sguid="IDirectMusicLoader8"; break; // {19e7c08c-0a44-4e6a-a116-595a7cd5de8c}

		// IDirectShow

		case 0x1E651CC0: sguid="CLSID_MemoryAllocator"; break; // 1E651CC0-B199-11D0-8212-00C04FC32C45
		case 0xbcde0395: sguid="CLSID_MMDeviceEnumerator"; break; // 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e
		case 0xE436EBB3: sguid="CLSID_FilterGraph"; break; // {E436EBB3-524F-11CE-9F53-0020AF0BA770}
		case 0x36b73882: sguid="IFilterGraph2"; break; // 36b73882-c2c8-11cf-8b46-00805f6cef60
		case 0xBF87B6E0: sguid="CLSID_CaptureGraphBuilder"; break; // {BF87B6E0-8C27-11d0-B3F0-00AA003761C5}
		case 0x5f2759c0: sguid="CLSID_AMovie"; break; // {5f2759c0-7685-11cf-8b23-00805f6cef60}
		case 0xe436ebb2: sguid="CLSID_FilterMapper"; break; // {e436ebb2-524f-11ce-9f53-0020af0ba770}
		case 0x62BE5D10: sguid="CLSID_SystemDeviceEnum"; break; // {62BE5D10-60EB-11d0-BD3B-00A0C911CE86}
		//case 0x9852A670: sguid="CLSID_DVobSubDecoder"; break; // CLSID_DVobSubDecoder : TGuid = '{9852A670-F845-491B-9BE6-EBD841B8A613}';
		case 0x9852A670: sguid="CLSID_VSFilter"; break; // {9852A670-F845-491B-9BE6-EBD841B8A613} DirectVobSub || VSFilter (auto-loading version)
		case 0x6BC1CFFA: sguid="CLSID_VideoRenderer"; break; // {6BC1CFFA-8FC1-4261-AC22-CFB4CC38DB50}
		case 0x56A868A9: sguid="IGraphBuilder"; break; // {56A868A9-0AD4-11CE-B03A-0020AF0BA770}
		case 0x56a868b4: sguid="IVideoWindow"; break; // "{56a868b4-0ad4-11ce-b03a-0020af0ba770")
		case 0x31ce832e: sguid="IVMRSurfaceAllocator"; break; // "31ce832e-4484-458b-8cca-f4d7e3db0b52"
		case 0x99D54F63: sguid="VMR Allocator Presenter"; break; // {99D54F63-1A69-41AE-AA4D-C976EB3F0713}
		case 0x8d5148ea: sguid="IVMRSurfaceAllocator9"; break; // (8d5148ea-3f5d-46cf-9df1-d1b896eedb1f)
		case 0x56a868b6: sguid="IMediaEvent interface"; break; // {56a868b6-0ad4-11ce-b03a-0020af0ba770}
		case 0x56a868a2: sguid="IMediaEventSink"; break; // {56a868a2-0ad4-11ce-b03a-0020af0ba770}
		case 0x632105FA: sguid="IAMGraphStreams"; break; // {632105FA-072E-11d3-8AF9-00C04FB6BD3D}
		case 0x62fae250: sguid="IAMOverlayFX"; break; // {62fae250-7e65-4460-bfc9-6398b322073c}
		case 0x56a868b5: sguid="IBasicVideo"; break; // {56a868b5-0ad4-11ce-b03a-0020af0ba770}
		case 0x329bb360: sguid="IBasicVideo2"; break; // (329bb360-f6ea-11d1-9038-00a0c9697298)
		case 0xACA12120: sguid="IDDVideoAcceleratorContainer"; break; // 0xACA12120, 0x3356, 0x11D1, 0x8F, 0xCF, 0x00, 0xC0, 0x4F, 0xC2, 0x9B, 0x4E
		case 0x56A868B2: sguid="IMediaPosition"; break; // 56A868B2-0AD4-11CE-B03A-0020AF0BA770 
		case 0x56a86893: sguid="IEnumFilters"; break; // ("56a86893-0ad4-11ce-b03a-0020af0ba770")
		case 0x56A868A6: sguid="IMediaFilter"; break; // 56A868A6-0AD4-11CE- B03A-0020AF0BA770
		case 0xa2104830: sguid="IFileSinkFilter"; break; // ("a2104830-7c70-11cf-8bce-00aa00a3f1a6")

		case 0xfeb50740: sguid="CLSID_CMpegVideoCodec"; break; // feb50740-7bef-11ce-9bd9-0000e202599c
		case 0xCF49D4E0: sguid="CLSID_AVIDec"; break; // {CF49D4E0-1115-11ce-B03A-0020AF0BA770} 
		case 0x1643e180: sguid="CLSID_Colour"; break; // {1643e180-90f5-11ce-97d5-00aa0055595a} 
		case 0x70e102b0: sguid="CLSID_VideoRenderer"; break; // {70e102b0-5556-11ce-97c0-00aa0055595a} 
		case 0x4a2286e0: sguid="CLSID_CMpegAudioCodec"; break; // {4a2286e0-7bef-11ce-9bd9-0000e202599c} 
		case 0xCE704FE7: sguid="IVMRImagePresenter"; break; // {CE704FE7-E71E-41FB-BAA2-C4403E1182F5}
		case 0x9F3A1C85: sguid="IVMRImagePresenterConfig"; break; // {9F3A1C85-8555-49BA-935F-BE5B5B29D178}
		case 0x9CF0B1B6: sguid="IVMRMonitorConfig"; break; // {9CF0B1B6-FBAA-4B7F-88CF-CF1F130A0DCE}
		case 0x0EB1088C: sguid="IVMRWindowlessControl"; break; // {0EB1088C-4DCD-46F0-878F-39DAE86A51B7}

		case 0xB98D13E7: sguid="CLSID_LAVSplitterSource"; break; // {B98D13E7-55DB-4385-A33D-09FD1BA26338} LAV Source
		case 0xC9361F5A: sguid="Ogg Demux Packet Source Filter"; break; // {C9361F5A-3282-4944-9899-6D99CDC5370B}
		case 0xAB9D6472: sguid="CLSID_RDPDShowRedirectionFilter"; break; //  {AB9D6472-752F-43F6-B29E-61207BDA8E06} - RDP Redirection Filter
		case 0xa490b1e4: sguid="IMFVideoDisplayControl"; break; // "a490b1e4-ab84-4d31-a1b2-181e03b1077a")
		case 0xfc4801a3: sguid="IObjectWithSite"; break; // fc4801a3-2ba9-11cf-a229-00aa003d7352 IObjectWithSite
		case 0x56a868a1: sguid="IID_Overlay"; break; // ("56a868a1-0ad4-11ce-b03a-0020af0ba770")
		case 0xC6E13343: sguid="IAMVideoCompression"; break; //("C6E13343-30AC-11d0-A18C-00A0C9118956")

		// FFDShow

		case 0x04FE9017: sguid="FFDShowVideo"; break; // "04FE9017-F873-410e-871E-AB91661A4EF7"
		case 0x0B390488: sguid="FFDShowVideoRaw"; break; // "0B390488-D80F-4a68-8408-48DC199F0E97"
		case 0x0B0EFF97: sguid="FFDShowVideoDXVA"; break; // "0B0EFF97-C750-462c-9488-B10E7D87F1A6"

// The used decoders are defined in MediaUriPlayer
//C87631F5-23BE-4986-8836-05832FCC48F9 IDirectMusicAudioPath
//9301E386-1F22-11D3-8226-D2FA76255D47 IDirectMusicContainer
//2252373A-5814-489B-8209-31FEDEBAF137 IDirectMusicScript
//51C22E10-B49F-46FC-BEC2-E6288FB9EDE6 IDirectMusicPatternTrack
//FD24AD8A-A260-453D-BF50-6F9384F70985 IDirectMusicStyle8
//A50E4730-0AE4-48A7-9839-BC04BFE07772 IDirectMusicSegmentState8
//C6784488-41A3-418F-AA15-B35093BA42D4 IDirectMusicSegment8
//679C4137-C62E-4147-B2B4-9D569ACB254C IDirectMusicPerformance8
//19E7C08C-0A44-4E6A-A116-595A7CD5DE8C IDirectMusicLoader8
//D38894D1-C052-11D2-872F-00600893B1BD IDirectMusicSegment2
//6FC2CAE0-BC78-11D2-AFA6-00AA0024D8B6 IDirectMusicPerformance2
//D2AC28C0-B39B-11D1-8704-00600893B1BD IDirectMusicBand
//D2AC28BF-B39B-11D1-8704-00600893B1BD IDirectMusicComposer
//D2AC28BE-B39B-11D1-8704-00600893B1BD IDirectMusicChordMap
//D2AC28BD-B39B-11D1-8704-00600893B1BD IDirectMusicStyle
//2BEFC277-5497-11D2-BCCB-00A0C922E6EB IDirectMusicGraph
//07D43D03-6523-11D2-871D-00600893B1BD IDirectMusicPerformance
//A3AFDCC7-D3EE-11D1-BC8D-00A0C922E6EB IDirectMusicSegmentState
//F96029A2-4282-11D2-8717-00600893B1BD IDirectMusicSegment
//D2AC28B5-B39B-11D1-8704-00600893B1BD IDirectMusicObject
//68A04844-D13D-11D1-AFA6-00AA0024D8B6 IDirectMusicGetLoader
//2FFAACA2-5DCA-11D2-AFA6-00AA0024D8B6 IDirectMusicLoader
//0E674304-3B05-11D3-9BD1-F9E7F0A01536 IDirectMusicTrack8
//0E674303-3B05-11D3-9BD1-F9E7F0A01536 IDirectMusicTool8
//D2AC28BA-B39B-11D1-8704-00600893B1BD IDirectMusicTool
//F96029A1-4282-11D2-8717-00600893B1BD IDirectMusicTrack
//2D3629F7-813D-4939-8508-F05C6B75FD97 IDirectMusic8
//6FC2CAE1-BC78-11D2-AFA6-00AA0024D8B6 IDirectMusic2
//D2AC287E-B39B-11D1-8704-00600893B1BD IDirectMusicDownloadedInstrument
//D2AC287D-B39B-11D1-8704-00600893B1BD IDirectMusicInstrument
//D2AC287C-B39B-11D1-8704-00600893B1BD IDirectMusicCollection
//D2AC287B-B39B-11D1-8704-00600893B1BD IDirectMusicDownload
//D2AC287A-B39B-11D1-8704-00600893B1BD IDirectMusicPortDownload
//CED153E7-3606-11D2-B9F9-0000F875AC12 IDirectMusicThru
//08F2D8C9-37C2-11D2-B9F9-0000F875AC12 IDirectMusicPort
//D2AC2878-B39B-11D1-8704-00600893B1BD IDirectMusicBuffer
//6536115A-7B2D-11D2-BA18-0000F875AC12 IDirectMusic

//E436EBB3-524F-11CE-9F53-0020AF0BA770 CLSID_FilterGraph
//BF87B6E0-8C27-11d0-B3F0-00AA003761C5 CLSID_CaptureGraphBuilder
//5f2759c0-7685-11cf-8b23-00805f6cef60 CLSID_AMovie
//e436ebb2-524f-11ce-9f53-0020af0ba770 CLSID_FilterMapper 

			/*
DEFINE_GUID(IID_IMediaStreamFilter, 0xbebe595e, 0x9a6f, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IDirectDrawMediaSampleAllocator, 0xab6b4afc, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IDirectDrawMediaSample, 0xab6b4afe, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IAMMediaTypeStream, 0xab6b4afa, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IAMMediaTypeSample, 0xab6b4afb, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IDirectDrawStreamSample, 0xf4104fcf, 0x9a70, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IDirectDrawMediaStream, 0xf4104fce, 0x9a70, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IDDVideoAcceleratorContainer, 0xACA12120, 0x3356, 0x11D1, 0x8F, 0xCF, 0x00, 0xC0, 0x4F, 0xC2, 0x9B, 0x4E);
DEFINE_GUID(IID_IDirectDrawVideoAccelerator, 0xC9B2D740, 0x3356, 0x11D1, 0x8F, 0xCF, 0x00, 0xC0, 0x4F, 0xC2, 0x9B, 0x4E);
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);

DEFINE_GUID(IID_IMMNotificationClient, 0x7991eec9, 0x7e89, 0x4d85, 0x83, 0x90, 0x6c, 0x70, 0x3c, 0xec, 0x60, 0xc0);
DEFINE_GUID(IID_IMMDevice, 0xd666063f, 0x1587, 0x4e43, 0x81, 0xf1, 0xb9, 0x48, 0xe8, 0x07, 0x36, 0x3f);
DEFINE_GUID(IID_IMMDeviceCollection, 0x0bd7a1be, 0x7a1a, 0x44db, 0x83, 0x97, 0xcc, 0x53, 0x92, 0x38, 0x7b, 0x5e);
DEFINE_GUID(IID_IMMEndpoint, 0x1be09788, 0x6894, 0x4089, 0x85, 0x86, 0x9a, 0x2a, 0x6c, 0x26, 0x5a, 0xc5);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);

DEFINE_GUID(IID_IMMDeviceActivator, 0x3b0d0ea4, 0xd0a9, 0x4b0e, 0x93, 0x5b, 0x09, 0x51, 0x67, 0x46, 0xfa, 0xc0);
DEFINE_GUID(IID_IActivateAudioInterfaceCompletionHandler, 0x41d949ab, 0x9862, 0x444a, 0x80, 0xf6, 0xc2, 0x61, 0x33, 0x4d, 0xa5, 0xeb);
DEFINE_GUID(IID_IActivateAudioInterfaceAsyncOperation, 0x72a22d78, 0xcde4, 0x431d, 0xb8, 0xcc, 0x84, 0x3a, 0x71, 0x19, 0x9b, 0x6d);
DEFINE_GUID(IID_IMultiMediaStream, 0xb502d1bc, 0x9a57, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IMediaStream, 0xb502d1bd, 0x9a57, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID(IID_IStreamSample, 0xb502d1be, 0x9a57, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
*/
		case 0xbebe595c: sguid="IAMMultiMediaStream"; break; // 0xbebe595c, 0x9a6f, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xbebe595d: sguid="IAMMediaStream"; break; // 0xbebe595d, 0x9a6f, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xbebe595e: sguid="IMediaStreamFilter"; break; // 0xbebe595e, 0x9a6f, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xab6b4afc: sguid="IDirectDrawMediaSampleAllocator"; break; // 0xab6b4afc, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xab6b4afe: sguid="IDirectDrawMediaSample"; break; // 0xab6b4afe, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xab6b4afa: sguid="IAMMediaTypeStream"; break; 
		case 0xab6b4afb: sguid="IAMMediaTypeSample"; break; // 0xab6b4afb, 0xf6e4, 0x11d0, 0x90, 0x0d, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xf4104fcf: sguid="IDirectDrawStreamSample"; break; // 0xf4104fcf, 0x9a70, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xf4104fce: sguid="IDirectDrawMediaStream"; break; // 0xf4104fce, 0x9a70, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xC9B2D740: sguid="IDirectDrawVideoAccelerator"; break; // 0xC9B2D740, 0x3356, 0x11D1, 0x8F, 0xCF, 0x00, 0xC0, 0x4F, 0xC2, 0x9B, 0x4E
		case 0x7991eec9: sguid="IMMNotificationClient"; break; // 0x7991eec9, 0x7e89, 0x4d85, 0x83, 0x90, 0x6c, 0x70, 0x3c, 0xec, 0x60, 0xc0
		case 0xd666063f: sguid="IMMDevice"; break; // 0xd666063f, 0x1587, 0x4e43, 0x81, 0xf1, 0xb9, 0x48, 0xe8, 0x07, 0x36, 0x3f
		case 0x0bd7a1be: sguid="IMMDeviceCollection"; break; // 0x0bd7a1be, 0x7a1a, 0x44db, 0x83, 0x97, 0xcc, 0x53, 0x92, 0x38, 0x7b, 0x5e
		case 0x1be09788: sguid="IMMEndpoint"; break; // 0x1be09788, 0x6894, 0x4089, 0x85, 0x86, 0x9a, 0x2a, 0x6c, 0x26, 0x5a, 0xc5
		case 0xa95664d2: sguid="IMMDeviceEnumerator"; break; // 0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6
		case 0x3b0d0ea4: sguid="IMMDeviceActivator"; break; // 0x3b0d0ea4, 0xd0a9, 0x4b0e, 0x93, 0x5b, 0x09, 0x51, 0x67, 0x46, 0xfa, 0xc0
		case 0x41d949ab: sguid="IActivateAudioInterfaceCompletionHandler"; break; // 0x41d949ab, 0x9862, 0x444a, 0x80, 0xf6, 0xc2, 0x61, 0x33, 0x4d, 0xa5, 0xeb
		case 0x72a22d78: sguid="IActivateAudioInterfaceAsyncOperation"; break; // 0x72a22d78, 0xcde4, 0x431d, 0xb8, 0xcc, 0x84, 0x3a, 0x71, 0x19, 0x9b, 0x6d
		case 0xb502d1bc: sguid="IMultiMediaStream"; break; // 0xb502d1bc, 0x9a57, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xb502d1bd: sguid="IMediaStream"; break; // 0xb502d1bd, 0x9a57, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d
		case 0xb502d1be: sguid="IStreamSample"; break; // 0xb502d1be, 0x9a57, 0x11d0, 0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d

		case 0x56A868B1: sguid="IMediaControl"; break; 
		case 0x36B73880: sguid="IMediaSeeking"; break; 
		case 0x56A868B3: sguid="IBasicAudio"; break; 
		case 0x56A8689C: sguid="IMemAllocator"; break; // {56A8689C-0AD4-11CE-B03A-0020AF0BA770}
		case 0x56A86895: sguid="IBaseFilter"; break; // {56A86895-0AD4-11CE-B03A-0020AF0BA770}
		case 0x56A8689F: sguid="IFilterGraph"; break; // {56A8689F-0AD4-11CE-B03A-0020AF0BA770}
		case 0x29840822: sguid="ICreateDevEnum"; break; // {29840822-5b84-11d0-bd3b-00a0c911ce86}

		case 0xAFB6C280: sguid="MPEG-2 Demultiplexer"; break;  // CLSID AFB6C280-2C41-11D3-8A60-0000F81E0E4A | MPEG-2 Demultiplexer
		case 0x280A3020: sguid="AC3 Parser Filter"; break; // CLSID 280A3020-86CF-11D1-ABE6-00A0C905F375 | AC3 Parser Filter
		case 0x1B544C20: sguid="AVI Splitter"; break; // CLSID 1B544C20-FD0B-11CE-8C63-00AA0044B51E | AVI Splitter
		case 0x336475D0: sguid="MPEG-I Stream Splitter"; break; // CLSID 336475D0-942A-11CE-A870-00AA002FEAB5 | MPEG-I Stream Splitter
		case 0x171252A0: sguid="CLAVSplitter"; break; // 171252A0-8820-4AFE-9DF8-5C92B2D66B04")) CLAVSplitter
		case 0x2dd74950: sguid="MPEGSplitter"; break; // {2dd74950-a890-11d1-abe8-00a0c905f375}

		case 0x7D8AA343: sguid="CLSID_VMR_ImageSync"; break; // {7D8AA343-6E63-4663-BE90-6B80F66540A3}

			// Filters ....

		case 0xf07e245f: sguid="CLSID_OggSplitter"; break; // {F07E245F-5A1F-4d1e-8BFF-DC31D84A55AB}
		case 0x078c3daa: sguid="CLSID_OggSplitPropPage"; break; // {078C3DAA-9E58-4d42-9E1C-7C8EE79539C5}
		case 0x8cae96b7: sguid="CLSID_OggMux"; break; // {8CAE96B7-85B1-4605-B23C-17FF5262B296}
		case 0xab97afc3: sguid="CLSID_OggMuxPropPage"; break; // {AB97AFC3-D08E-4e2d-98E0-AEE6D4634BA4}
		case 0x889ef574: sguid="CLSID_VorbisEnc"; break; // {889EF574-0656-4b52-9091-072E52BB1B80}
		case 0xc5379125: sguid="CLSID_VorbisEncPropPage"; break; // {c5379125-fd36-4277-a7cd-fab469ef3a2f}
		case 0x02391f44: sguid="CLSID_VorbisDec"; break; // {02391f44-2767-4e6a-a484-9b47b506f3a4}
		case 0x77983549: sguid="CLSID_OggDSAboutPage"; break; // {77983549-ffda-4a88-b48f-b924e8d1f01c}
		case 0x6b6d0800: sguid="CLSID_NetShowSource"; break; // {6B6D0800-9ADA-11d0-A520-00A0D10129C0}
		case 0x564FD788: sguid="CLSID_HaaliSplitterAR"; break; // {564FD788-86C9-4444-971E-CC4A243DA150} Haali Splitter (AR)
		case 0x55DA30FC: sguid="CLSID_HaaliSplitter"; break; // {55DA30FC-F16B-49FC-BAA5-AE59FC65F82D} Haali Splitter
		case 0x212690FB: sguid="CLSID_CMPEG2VidDecoderDS"; break; // {212690FB-83E5-4526-8FD7-74478B7939CD} from wmcodecdsp.h
		case 0x71E4616A: sguid="CLSID_NvidiaVideoDecoder"; break; // {71E4616A-DB5E-452B-8CA5-71D9CC7805E9} - Nvidia VideoDecoder
		case 0xD7D50E8D: sguid="CLSID_SonicCinemasterVideoDecoder"; break; // {D7D50E8D-DD72-43C2-8587-A0C197D837D2} - Sonic Cinemaster VideoDecoder
		case 0xA753A1EC: sguid="CLSID_AC3Filter"; break; // {A753A1EC-973E-4718-AF8E-A3F554D45C44}
		case 0x495cf191: sguid="CLSID_VMR7AllocatorPresenter"; break; // {495CF191-810D-44c7-92C5-E7D46AE00F44}
		case 0x97b3462e: sguid="CLSID_RM7AllocatorPresenter"; break; // {97B3462E-1752-4dfb-A038-271060BC7A94}
		case 0x36cc5a71: sguid="CLSID_QT7AllocatorPresenter"; break; // {36CC5A71-441C-462a-9D10-48A19485938D}
		case 0x4e4834fa: sguid="CLSID_VMR9AllocatorPresenter"; break; // {4E4834FA-22C2-40e2-9446-F77DD05D245E}
		case 0xa1542f93: sguid="CLSID_RM9AllocatorPresenter"; break; // {A1542F93-EB53-4e11-8D34-05C57ABA9207}
		case 0x622a4032: sguid="CLSID_QT9AllocatorPresenter"; break; // {622A4032-70CE-4040-8231-0F24F2886618}
		case 0x7612b889: sguid="CLSID_EVRAllocatorPresenter"; break; // {7612b889-e070-4bcc-0b88-91cb794174ab}
		case 0xf9f62627: sguid="CLSID_SyncAllocatorPresenter"; break; // {F9F62627-E3EF-4a2e-B6C9-5D4C0DC3326B}
		case 0xb72ebdd4: sguid="CLSID_DXRAllocatorPresenter"; break; // {B72EBDD4-831D-440f-A656-B48F5486CD82}
		case 0xc7ed3100: sguid="CLSID_madVRAllocatorPresenter"; break; // {C7ED3100-9002-4595-9DCA-B30B30413429}

		case 0x760a8f35: sguid="CLSID_DXR"; break; // {760A8F35-97E7-479d-AAF5-DA9EFF95D751} Haali's video renderer
		case 0xe1a8b82a: sguid="CLSID_madVR"; break; // {E1A8B82A-32CE-4B0D-BE0D-AA68C772E423} madVR
		case 0x601d2a2b: sguid="CLSID_MpcAudioRenderer"; break; // {601D2A2B-9CDE-40bd-8650-0485E3522727}
		case 0x9dc15360: sguid="CLSID_ReClock"; break; // {9DC15360-914C-46B8-B9DF-BFE67FD36C6A} - ReClock
		case 0xD3CD7858: sguid="CLSID_MorganSwitcher"; break; // {D3CD7858-971A-4838-ACEC-40CA5D529DC8} - Morgan's Stream Switcher
		case 0xB86F6BEE: sguid="CLSID_ffdshowAudioProcessor"; break; // {B86F6BEE-E7C0-4D03-8D52-5B4430CF6C88} - ffdshow Audio Processor
		case 0x482d10b6: sguid="CLSID_RatDVDNavigator"; break; // {482d10b6-376e-4411-8a17-833800A065DB}" XEB Navigation Filter (RatDVD)
		case 0x2DFCB782: sguid="CLSID_XySubFilter"; break; // {2DFCB782-EC20-4A7C-B530-4577ADB33F21} XySubFilter
		case 0x6B237877: sguid="CLSID_XySubFilter_AutoLoader"; break; // {6B237877-902B-4C6C-92F6-E63169A5166C} XySubFilterAutoLoader
		case 0x37D84F60: sguid="IPersistPropertyBag"; break; // {37D84F60-42CB-11CE-8135-00AA004BB851}
		case 0x8E1C39A1: sguid="IID_IAMOpenProgress"; break; // {8E1C39A1-DE53-11cf-AA63-0080C744528D}
		case 0xF90A6130: sguid="IAMDeviceRemoval"; break; // F90A6130-B658-11D2-AE49-0000F8754B99 
		case 0xEBE1FB08: sguid="IDirectVobSub"; break; 	// EBE1FB08-3957-47ca-AF13-5827E5442E56
		// DXGI
//DEFINE_GUID(IID_IDXGIObject,0xaec22fb8,0x76f3,0x4639,0x9b,0xe0,0x28,0xeb,0x43,0xa6,0x7a,0x2e);
//DEFINE_GUID(IID_IDXGIDeviceSubObject,0x3d3e0379,0xf9de,0x4d58,0xbb,0x6c,0x18,0xd6,0x29,0x92,0xf1,0xa6);
//DEFINE_GUID(IID_IDXGIResource,0x035f3ab4,0x482e,0x4e50,0xb4,0x1f,0x8a,0x7f,0x8b,0xd8,0x96,0x0b);
//DEFINE_GUID(IID_IDXGISurface,0xcafcb56c,0x6ac3,0x4889,0xbf,0x47,0x9e,0x23,0xbb,0xd2,0x60,0xec);
//DEFINE_GUID(IID_IDXGIAdapter,0x2411e7e1,0x12ac,0x4ccf,0xbd,0x14,0x97,0x98,0xe8,0x53,0x4d,0xc0);
//DEFINE_GUID(IID_IDXGIOutput,0xae02eedb,0xc735,0x4690,0x8d,0x52,0x5a,0x8d,0xc2,0x02,0x13,0xaa);
//DEFINE_GUID(IID_IDXGISwapChain,0x310d36a0,0xd2e7,0x4c0a,0xaa,0x04,0x6a,0x9d,0x23,0xb8,0x88,0x6a);
//DEFINE_GUID(IID_IDXGIFactory,0x7b7166ec,0x21c7,0x44ae,0xb2,0x1a,0xc9,0xae,0x32,0x1a,0xe3,0x69);
//DEFINE_GUID(IID_IDXGIDevice,0x54ec77fa,0x1377,0x44e6,0x8c,0x32,0x88,0xfd,0x5f,0x44,0xc8,0x4c);

//DEFINE_GUID(IID_IDXGIObject,0xaec22fb8,0x76f3,0x4639,0x9b,0xe0,0x28,0xeb,0x43,0xa6,0x7a,0x2e);
//DEFINE_GUID(IID_IDXGIDeviceSubObject,0x3d3e0379,0xf9de,0x4d58,0xbb,0x6c,0x18,0xd6,0x29,0x92,0xf1,0xa6);
//DEFINE_GUID(IID_IDXGIResource,0x035f3ab4,0x482e,0x4e50,0xb4,0x1f,0x8a,0x7f,0x8b,0xd8,0x96,0x0b);
//DEFINE_GUID(IID_IDXGIKeyedMutex,0x9d8e1289,0xd7b3,0x465f,0x81,0x26,0x25,0x0e,0x34,0x9a,0xf8,0x5d);
//DEFINE_GUID(IID_IDXGISurface,0xcafcb56c,0x6ac3,0x4889,0xbf,0x47,0x9e,0x23,0xbb,0xd2,0x60,0xec);
//DEFINE_GUID(IID_IDXGISurface1,0x4AE63092,0x6327,0x4c1b,0x80,0xAE,0xBF,0xE1,0x2E,0xA3,0x2B,0x86);
//DEFINE_GUID(IID_IDXGIAdapter,0x2411e7e1,0x12ac,0x4ccf,0xbd,0x14,0x97,0x98,0xe8,0x53,0x4d,0xc0);
//DEFINE_GUID(IID_IDXGIOutput,0xae02eedb,0xc735,0x4690,0x8d,0x52,0x5a,0x8d,0xc2,0x02,0x13,0xaa);
//DEFINE_GUID(IID_IDXGISwapChain,0x310d36a0,0xd2e7,0x4c0a,0xaa,0x04,0x6a,0x9d,0x23,0xb8,0x88,0x6a);
//DEFINE_GUID(IID_IDXGIFactory,0x7b7166ec,0x21c7,0x44ae,0xb2,0x1a,0xc9,0xae,0x32,0x1a,0xe3,0x69);
//DEFINE_GUID(IID_IDXGIDevice,0x54ec77fa,0x1377,0x44e6,0x8c,0x32,0x88,0xfd,0x5f,0x44,0xc8,0x4c);
//DEFINE_GUID(IID_IDXGIFactory1,0x770aae78,0xf26f,0x4dba,0xa8,0x29,0x25,0x3c,0x83,0xd1,0xb3,0x87);
//DEFINE_GUID(IID_IDXGIAdapter1,0x29038f61,0x3839,0x4626,0x91,0xfd,0x08,0x68,0x79,0x01,0x1a,0x05);
//DEFINE_GUID(IID_IDXGIDevice1,0x77db970f,0x6276,0x48ba,0xba,0x28,0x07,0x01,0x43,0xb4,0x39,0x2c);
//DEFINE_GUID(IID_IDXGIFactory2, 0x50c83a1c, 0xe072, 0x4c48, 0x87,0xb0, 0x36,0x30,0xfa,0x36,0xa6,0xd0);
//IID_IDXGIDevice2: TGUID = '{05008617-fbfd-4051-a790-144884b4f6a9}';
//IID_IDXGIAdapter2: TGUID = '{0AA1AE0A-FA0E-4B84-8644-E05FF8E5ACB5}';
//DEFINE_GUID (IID_IDXGIOutput5, 0x80A07424, 0xAB52, 0x42EB, 0x83, 0x3C, 0x0C, 0x42, 0xFD, 0x28, 0x2D, 0x98)
//DEFINE_GUID (IID_IDXGISwapChain4, 0x3D585D5A, 0xBD4A, 0x489E, 0xB1, 0xF4, 0x3D, 0xBC, 0xB6, 0x45, 0x2F, 0xFB)
//DEFINE_GUID (IID_IDXGIDevice4, 0x95B4F95F, 0xD8DA, 0x4CA4, 0x9E, 0xE6, 0x3B, 0x76, 0xD5, 0x96, 0x8A, 0x10)
//DEFINE_GUID (IID_IDXGIFactory5, 0x7632e1f5, 0xee65, 0x4dca, 0x87, 0xfd, 0x84, 0xcd, 0x75, 0xf8, 0x83, 0x8d)
//IID_IDXGIDevice3: TGUID = '{6007896c-3244-4afd-bf18-a6d3beda5023}';
//IID_IDXGISwapChain2: TGUID = '{a8be2ac4-199f-4946-b331-79599fb98de7}';
//IID_IDXGIOutput2: TGUID = '{595e39d1-2724-4663-99b1-da969de28364}';
//IID_IDXGIFactory3: TGUID = '{25483823-cd46-4c7d-86ca-47aa95b837bd}';
//IID_IDXGIDecodeSwapChain: TGUID = '{2633066b-4514-4c7a-8fd8-12ea98059d18}';
//IID_IDXGIFactoryMedia: TGUID = '{41e7d1f2-a591-4f7b-a2e5-fa9c843e1c12}';
//IID_IDXGISwapChainMedia: TGUID = '{dd95b90b-f05f-4f6a-bd65-25bfb264bd84}';
//IID_IDXGIOutput3: TGUID = '{8a6bb301-7e7e-41F4-a8e0-5b32f7f99b18}';
//DEFINE_GUID(IID_IDXGIOutput2,0x595e39d1,0x2724,0x4663,0x99,0xb1,0xda,0x96,0x9d,0xe2,0x83,0x64);
//DEFINE_GUID(IID_IDXGIDevice3,0x6007896c,0x3244,0x4afd,0xbf,0x18,0xa6,0xd3,0xbe,0xda,0x50,0x23);
//DEFINE_GUID(IID_IDXGISwapChain2,0xa8be2ac4,0x199f,0x4946,0xb3,0x31,0x79,0x59,0x9f,0xb9,0x8d,0xe7);
//DEFINE_GUID(IID_IDXGIFactory3,0x25483823,0xcd46,0x4c7d,0x86,0xca,0x47,0xaa,0x95,0xb8,0x37,0xbd);
//DEFINE_GUID(IID_IDXGIDecodeSwapChain,0x2633066b,0x4514,0x4c7a,0x8f,0xd8,0x12,0xea,0x98,0x05,0x9d,0x18);
//DEFINE_GUID(IID_IDXGIFactoryMedia,0x41e7d1f2,0xa591,0x4f7b,0xa2,0xe5,0xfa,0x9c,0x84,0x3e,0x1c,0x12);
//DEFINE_GUID(IID_IDXGISwapChainMedia,0xdd95b90b,0xf05f,0x4f6a,0xbd,0x65,0x25,0xbf,0xb2,0x64,0xbd,0x84);
//DEFINE_GUID(IID_IDXGIOutput3,0x8a6bb301,0x7e7e,0x41F4,0xa8,0xe0,0x5b,0x32,0xf7,0xf9,0x9b,0x18);	 	
//IID_IDXGISwapChain3: TGUID = '{94d99bdb-f1f8-4ab0-b236-7da0170edab1}';
//IID_IDXGIOutput4: TGUID = '{dc7dca35-2196-414d-9F53-617884032a60}';
//IID_IDXGIFactory4: TGUID = '{1bc6ea02-ef36-464f-bf0c-21ca39e5168a}';
//IID_IDXGIAdapter3: TGUID = '{645967A4-1392-4310-A798-8053CE3E93FD}';
//IID_IDXGIDisplayControl: TGUID = '{ea9dbf1a-c88e-4486-854a-98aa0138f30c}';
//IID_IDXGIOutputDuplication: TGUID = '{191cfac3-a341-470d-b26e-a864f428319c}';
//IID_IDXGISurface2: TGUID = '{aba496dd-b617-4cb8-a866-bc44d7eb1fa2}';
//IID_IDXGIResource1: TGUID = '{30961379-4609-4a41-998e-54fe567ee0c1}';
//IID_IDXGIDevice2: TGUID = '{05008617-fbfd-4051-a790-144884b4f6a9}';
//IID_IDXGISwapChain1: TGUID = '{790a45f7-0d42-4876-983a-0a55cfe6f4aa}';
//IID_IDXGIFactory2: TGUID = '{50c83a1c-e072-4c48-87b0-3630fa36a6d0}';
//IID_IDXGIAdapter2: TGUID = '{0AA1AE0A-FA0E-4B84-8644-E05FF8E5ACB5}';
//IID_IDXGIOutput1: TGUID = '{00cddea8-939b-4b83-a340-a685226666cc}';
//DEFINE_GUID(IID_IDXGIAdapter4,0x3c8d99d1,0x4fbf,0x4181,0xa8,0x2c,0xaf,0x66,0xbf,0x7b,0xd2,0x4e);
//DEFINE_GUID(IID_IDXGIOutput6,0x068346e8,0xaaec,0x4b84,0xad,0xd7,0x13,0x7f,0x51,0x3f,0x77,0xa1);

		case 0xaec22fb8: sguid="IID_IDXGIObject"; break;
		case 0x3d3e0379: sguid="IID_IDXGIDeviceSubObject"; break;
		case 0x191cfac3: sguid="IID_IDXGIOutputDuplication"; break;
		case 0xea9dbf1a: sguid="IID_IDXGIDisplayControl"; break;
		case 0x035f3ab4: sguid="IID_IDXGIResource"; break;
		case 0x30961379: sguid="IID_IDXGIResource1"; break;
		case 0xcafcb56c: sguid="IID_IDXGISurface"; break;
		case 0x4AE63092: sguid="IID_IDXGISurface1"; break;
		case 0xaba496dd: sguid="IID_IDXGISurface2"; break;
		case 0xae02eedb: sguid="IID_IDXGIOutput"; break;
		case 0x00cddea8: sguid="IID_IDXGIOutput1"; break;
		case 0x595e39d1: sguid="IID_IDXGIOutput2"; break;
		case 0x8a6bb301: sguid="IID_IDXGIOutput3"; break;
		case 0xdc7dca35: sguid="IID_IDXGIOutput4"; break;
		case 0x80A07424: sguid="IID_IDXGIOutput5"; break;
		case 0x068346e8: sguid="IID_IDXGIOutput6"; break;
		case 0x310d36a0: sguid="IID_IDXGISwapChain"; break;
		case 0x790a45f7: sguid="IID_IDXGISwapChain1"; break;
		case 0xa8be2ac4: sguid="IID_IDXGISwapChain2"; break;
		case 0x94d99bdb: sguid="IID_IDXGISwapChain3"; break;
		case 0x3D585D5A: sguid="IID_IDXGISwapChain4"; break;
		case 0x7b7166ec: sguid="IID_IDXGIFactory"; break;
		case 0x770aae78: sguid="IID_IDXGIFactory1"; break;
		case 0x50c83a1c: sguid="IID_IDXGIFactory2"; break;
		case 0x25483823: sguid="IID_IDXGIFactory3"; break;
		case 0x1bc6ea02: sguid="IID_IDXGIFactory4"; break;
		case 0x7632e1f5: sguid="IID_IDXGIFactory5"; break;
		case 0x2411e7e1: sguid="IID_IDXGIAdapter"; break;
		case 0x29038f61: sguid="IID_IDXGIAdapter1"; break;
		case 0x0AA1AE0A: sguid="IID_IDXGIAdapter2"; break;
		case 0x645967A4: sguid="IID_IDXGIAdapter3"; break;
		case 0x3c8d99d1: sguid="IID_IDXGIAdapter4"; break;
		case 0x54ec77fa: sguid="IID_IDXGIDevice"; break;
		case 0x77db970f: sguid="IID_IDXGIDevice1"; break;
		case 0x05008617: sguid="IID_IDXGIDevice2"; break;
		case 0x6007896c: sguid="IID_IDXGIDevice3"; break;
		case 0x95B4F95F: sguid="IID_IDXGIDevice4"; break;
		case 0x9d8e1289: sguid="IID_IDXGIKeyedMutex"; break;
		case 0x7abb6563: sguid="IID_IDXGIAdapterInternal2"; break; // from apitrace guids_entries.h
		case 0x2633066b: sguid="IID_IDXGIDecodeSwapChain"; break; 
		case 0x41e7d1f2: sguid="IID_IDXGIFactoryMedia"; break; 
		case 0xdd95b90b: sguid="IID_IDXGISwapChainMedia"; break; 
		case 0xF0DB4C7F: sguid="IID_IDXCoreAdapter"; break; // F0DB4C7F-FE5A-42A2-BD62-F2A6CF6FC83E

		// to classify:

		case 0xE436EBB5: sguid="IBaseFilter"; break; // E436EBB5-524F-11CE-9F53-0020AF0BA770

		// components in registry

		case 0xE1211353: sguid="DIRECT.DirectX6.0"; break; // E1211353-8E94-11D1-8808-00C04FC2C602
		case 0xE1211242: sguid="DirectX 7 for Visual Basic Type Library"; break; //E1211242-8E94-11D1-8808-00C04FC2C602

		// default: unknown

		default: sguid="UNKNOWN"; break;
	}
	return sguid;
}

