#include "pch.h"
#include "BitmapDefinition.h"

BitmapDefinition::BitmapDefinition(PCWSTR uri) : uri(uri) {}

void BitmapDefinition::CreateDeviceIndependentResources(IWICImagingFactory* imaging_factory) {
	winrt::com_ptr<IWICBitmapDecoder> decoder;
	winrt::com_ptr<IWICBitmapFrameDecode> source;
	winrt::com_ptr<IWICStream> stream;

	winrt::check_hresult(imaging_factory->CreateDecoderFromFilename(
		uri,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		decoder.put()
	));

	winrt::check_hresult(decoder->GetFrame(0, source.put()));

	winrt::check_hresult(imaging_factory->CreateFormatConverter(converter.put()));

	winrt::check_hresult(converter->Initialize(
		source.get(),
		GUID_WICPixelFormat32bppRGBA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.f,
		WICBitmapPaletteTypeMedianCut
	));
}

BYTE* BitmapDefinition::GetBitmapAsBytes(UINT* width, UINT* height) {
	winrt::check_hresult(converter->GetSize(width, height));

	BYTE* bits = new BYTE[4 * *width * *height];
	winrt::check_hresult(converter->CopyPixels(nullptr, 4 * *width, 4 * *width * *height, bits));

	return bits;
}
