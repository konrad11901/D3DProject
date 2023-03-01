#pragma once

class BitmapDefinition {
public:
	BitmapDefinition(PCWSTR uri);
	void CreateDeviceIndependentResources(IWICImagingFactory* imaging_factory);
	BYTE* GetBitmapAsBytes(UINT* width, UINT* height);
private:
	PCWSTR uri;
	winrt::com_ptr<IWICFormatConverter> converter;
};
