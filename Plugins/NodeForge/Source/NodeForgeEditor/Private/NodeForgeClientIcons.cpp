#include "NodeForgePanelPreferences.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <wincodec.h>
#include <wrl/client.h>
#include "Windows/HideWindowsPlatformTypes.h"

bool FNodeForgePanelPreferences::DecodeIcon(const TArray<uint8>& Bytes,TArray<uint8>& Png,FString& Error)
{
    using Microsoft::WRL::ComPtr;
    Png.Empty();Error.Empty();
    if(Bytes.IsEmpty()||Bytes.Num()>204800){Error=TEXT("图标请使用不超过200 KB的PNG、JPEG或WebP。");return false;}
    const HRESULT Initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    struct FComScope{bool Owned;~FComScope(){if(Owned)CoUninitialize();}} Scope{SUCCEEDED(Initialized)};
    if(FAILED(Initialized)&&Initialized!=RPC_E_CHANGED_MODE){Error=TEXT("系统图像解码服务不可用。");return false;}
    ComPtr<IWICImagingFactory> Factory;ComPtr<IWICStream> Input;
    ComPtr<IWICBitmapDecoder> Decoder;ComPtr<IWICBitmapFrameDecode> Frame;
    auto Fail=[&](){Png.Empty();Error=TEXT("图标不是可解码的PNG、JPEG或WebP，或系统缺少该图像解码器。");return false;};
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(Factory.GetAddressOf())))
        ||FAILED(Factory->CreateStream(Input.GetAddressOf()))
        ||FAILED(Input->InitializeFromMemory(const_cast<uint8*>(Bytes.GetData()),Bytes.Num()))
        ||FAILED(Factory->CreateDecoderFromStream(Input.Get(),nullptr,WICDecodeMetadataCacheOnLoad,Decoder.GetAddressOf())))return Fail();
    GUID Container={};UINT Count=0,Width=0,Height=0;
    if(FAILED(Decoder->GetContainerFormat(&Container))
        ||(Container!=GUID_ContainerFormatPng&&Container!=GUID_ContainerFormatJpeg&&Container!=GUID_ContainerFormatWebp)
        ||FAILED(Decoder->GetFrameCount(&Count))||Count!=1||FAILED(Decoder->GetFrame(0,Frame.GetAddressOf()))
        ||FAILED(Frame->GetSize(&Width,&Height))||Width<1||Height<1||Width>4096||Height>4096)return Fail();
    ComPtr<IWICBitmapScaler> Scaler;ComPtr<IWICFormatConverter> Converter;
    const double Scale=FMath::Min(1.0,256.0/FMath::Max(Width,Height));
    Width=FMath::Max(1u,UINT(Width*Scale));Height=FMath::Max(1u,UINT(Height*Scale));
    if(FAILED(Factory->CreateBitmapScaler(Scaler.GetAddressOf()))
        ||FAILED(Scaler->Initialize(Frame.Get(),Width,Height,WICBitmapInterpolationModeFant))
        ||FAILED(Factory->CreateFormatConverter(Converter.GetAddressOf()))
        ||FAILED(Converter->Initialize(Scaler.Get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return Fail();
    ComPtr<IStream> Output;ComPtr<IWICBitmapEncoder> Encoder;ComPtr<IWICBitmapFrameEncode> EncodedFrame;ComPtr<IPropertyBag2> Options;
    WICPixelFormatGUID Pixel=GUID_WICPixelFormat32bppBGRA;
    if(FAILED(CreateStreamOnHGlobal(nullptr,1,Output.GetAddressOf()))
        ||FAILED(Factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,Encoder.GetAddressOf()))
        ||FAILED(Encoder->Initialize(Output.Get(),WICBitmapEncoderNoCache))
        ||FAILED(Encoder->CreateNewFrame(EncodedFrame.GetAddressOf(),Options.GetAddressOf()))
        ||FAILED(EncodedFrame->Initialize(Options.Get()))||FAILED(EncodedFrame->SetSize(Width,Height))
        ||FAILED(EncodedFrame->SetPixelFormat(&Pixel))||FAILED(EncodedFrame->WriteSource(Converter.Get(),nullptr))
        ||FAILED(EncodedFrame->Commit())||FAILED(Encoder->Commit()))return Fail();
    STATSTG Stats={};LARGE_INTEGER Zero={};ULONG Read=0;
    if(FAILED(Output->Stat(&Stats,STATFLAG_NONAME))||Stats.cbSize.QuadPart<1||Stats.cbSize.QuadPart>204800
        ||FAILED(Output->Seek(Zero,STREAM_SEEK_SET,nullptr)))return Fail();
    Png.SetNumUninitialized(int32(Stats.cbSize.QuadPart));
    if(FAILED(Output->Read(Png.GetData(),Png.Num(),&Read))||Read!=ULONG(Png.Num()))return Fail();
    return true;
}
