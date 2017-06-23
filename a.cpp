#include <iostream>
#include <cmath>
#include <wincodec.h>
#include <Shlwapi.h>

#pragma comment(lib, "WindowsCodecs.lib")
#pragma comment(lib, "Shlwapi.lib")

template <class T>
void SafeRelease(T **ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

class ThrowHR {
public:
    ThrowHR() : hr(S_OK) {}
    HRESULT operator=(HRESULT hr) {
        this->hr = hr;
        if (FAILED(hr)) {
            std::cout << this->hr << std::endl;
            throw this->hr;
        }
        return this->hr;
    }
    HRESULT get() const {
        return hr;
    }

private:
    HRESULT hr;
};

int mandel(const double a, const double b) {
    double x = a, y = b;
    double r2, t, c;
    int i, k = 3;

    for (i = 0; i < k; i++) {
        t = x * x - y * y + a;
        y = 2 * x * y + b;
        x = t;
        r2 = x * x + y * y;
        if (r2 > 1e+100) { i++; break; }
    }
    // kÇÕç≈ëÂîΩïúâÒêîÅAiÇÕé¿ç€ÇÃîΩïúâÒêî

    // log(log(r))Ç≈êFÇïtÇØÇÈ
    if (r2 == 0.0) return 0;
    c = log(r2) * 0.5; // log(r)
    if (c == 0.0) return 255;
    c = log(fabs(c)) + log(2.0) * (k - i);

    // kÇ…âûÇ∂ÇƒcÇìKìñÇ…â¡çH
    c = ceil(-64.0 * c);
    if (c <= 0.0) return 0;
    if (c >= 255.0) return 255;
    return (int)c;
}

HRESULT draw() {
    IWICImagingFactory *factory = nullptr;
    IWICBitmapDecoder *decoder = nullptr;
    IWICBitmapFrameDecode *frameDecode = nullptr;
    IWICBitmapEncoder *encoder = nullptr;
    IWICBitmapFrameEncode *frameEncode = nullptr;
    IWICBitmap *bitmap = nullptr;
    IStream *stream = nullptr;
    BYTE *p0 = nullptr, *q0 = nullptr;
    GUID pf = GUID_WICPixelFormat32bppBGRA;

    ThrowHR hr;
    try {
        hr = CoInitialize(nullptr);
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory, reinterpret_cast<void **>(&factory));

        // âHÇì«Ç›çûÇﬁ
        UINT width, height, stride, size;
        const UINT Bpp = 4;
        hr = factory->CreateDecoderFromFilename(L"angel.png", nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        hr = decoder->GetFrame(0, &frameDecode);
        hr = frameDecode->GetSize(&width, &height);

        stride = (width * Bpp + 3) & ~3;
        size = stride * height;
        q0 = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
        if (!q0) throw hr = -1;
        hr = frameDecode->CopyPixels(nullptr, stride, size, q0);

        // âHâÊëúÇ∆ìØÇ∂ÉTÉCÉYÇÃäGÇï`Ç≠
        p0 = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size);
        if (!p0) throw hr = -1;

        double x0 = (width - 1) * 0.5, y0 = (height - 1) * 0.5, s0 = 1.3 / x0;
        y0 *= 1.55; // ècÇ…Ç∏ÇÁÇ∑

        auto divr = [](UINT a1, UINT a2) -> UINT {
            return (a1 + a2 / 2) / a2;
        };

        for (UINT j = 0; j < height; j++) {
            BYTE *p = p0 + stride * j;
            BYTE *q = q0 + stride * (j - 80); // âHÇâ∫Ç…Ç∏ÇÁÇ∑
            for (UINT i = 0; i < width; i++) {
                // BGRA
                p[0] = 0xb7;
                p[1] = 0x79;
                p[2] = 0x4a;
                p[3] = mandel((j - y0) * s0, (i - x0) * s0);

                if (180 < j && j < 330 && q[3] > 0) {
                    // âH
                    UINT tq = q[3] * 255;
                    // mandel
                    UINT tp = (255 - q[3]) * p[3];
                    // îwåi
                    UINT tb = (255 - q[3]) * (255 - p[3]);
                    // tq + tp + tb == 255 * 255
                    for (int k = 0; k < 3; k++) {
                        p[k] = divr(p[k] * tp + q[k] * tq, tp + tq);
                    }
                    p[3] = 255 - divr(tb, 255);
                }
                p += Bpp;
                q += Bpp;
            }
        }
        hr = factory->CreateBitmapFromMemory(width, height, pf, stride, size, p0, &bitmap);

        hr = factory->CreateEncoder(GUID_ContainerFormatPng, 0, &encoder);
        hr = SHCreateStreamOnFileEx(L"out.png", STGM_WRITE | STGM_CREATE, 0, TRUE, nullptr, &stream);
        hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
        hr = encoder->CreateNewFrame(&frameEncode, nullptr);

        hr = frameEncode->Initialize(nullptr);
        hr = frameEncode->SetPixelFormat(&pf);
        hr = frameEncode->SetSize(width, height);
        hr = frameEncode->WriteSource(bitmap, nullptr);
        hr = frameEncode->Commit();
        hr = encoder->Commit();

    } catch (HRESULT) {}

    if (q0) HeapFree(GetProcessHeap(), 0, q0);
    if (p0) HeapFree(GetProcessHeap(), 0, p0);
    SafeRelease(&stream);
    SafeRelease(&bitmap);
    SafeRelease(&frameEncode);
    SafeRelease(&encoder);
    SafeRelease(&frameDecode);
    SafeRelease(&decoder);
    SafeRelease(&factory);
    CoUninitialize();

    return hr.get();
}

int main() {
    draw();
    return 0;
}
