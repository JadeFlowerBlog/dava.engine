#include "TexturePacker/ImageExt.h"

#include <CommandLine/CommandLineParser.h>
#include <Render/Image/ImageSystem.h>
#include <Render/Image/ImageConvert.h>
#include <Render/Texture.h>
#include <Render/PixelFormatDescriptor.h>

namespace DAVA
{
ImageExt::ImageExt()
    : internalData(nullptr)
{
}

ImageExt::ImageExt(const ImageExt& img)
    : internalData(nullptr)
{
    if (img.internalData)
    {
        internalData.reset(Image::CreateFromData(img.internalData->GetWidth(), img.internalData->GetHeight(), img.internalData->GetPixelFormat(),
                                                 img.internalData->GetData()));
    }
}

ImageExt::~ImageExt()
{
}

bool ImageExt::Read(const FilePath& filename)
{
    internalData.reset(ImageSystem::LoadSingleMip(filename, 0));
    if (internalData.get() == nullptr)
    {
        Logger::Error("[ImageExt::Read] failed to read image file: %s", filename.GetAbsolutePathname().c_str());
        return false;
    }
    if (internalData->format != FORMAT_RGBA8888 &&
        ConvertToFormat(FORMAT_RGBA8888) == false)
    {
        Logger::Error("[ImageExt::Read] failed to convert file to RGBA8888");
        return false;
    }
    return true;
}

void ImageExt::Write(const FilePath& filename, ImageQuality quality)
{
    DVASSERT(internalData);
    ImageSystem::Save(filename, internalData, internalData->format, quality);
}

bool ImageExt::Create(uint32 width, uint32 height)
{
    internalData = Image::Create(width, height, FORMAT_RGBA8888);
    if (internalData)
    {
        Memset(internalData->data, 0, internalData->dataSize);
        return true;
    }

    return false;
}

bool ImageExt::ConvertToFormat(PixelFormat newFormat)
{
    DVASSERT(internalData);
    if (internalData->format == newFormat)
    {
        return true;
    }

    ScopedPtr<Image> newImage(Image::Create(GetWidth(), GetHeight(), newFormat));
    bool convertResult = ImageConvert::ConvertImageDirect(internalData, newImage);

    if (convertResult == true)
    {
        internalData = newImage;
    }

    return convertResult;
}

void ImageExt::DrawImage(int32 sx, int32 sy, ImageExt* image, const Rect2i& srcRect)
{
    uint32* destData32 = reinterpret_cast<uint32*>(GetData());
    uint32* srcData32 = reinterpret_cast<uint32*>(image->GetData());

    int32 rx, ry;
    ry = sy;
    for (int32 y = srcRect.y; y < srcRect.y + srcRect.dy; ++y)
    {
        rx = sx;
        for (int32 x = srcRect.x; x < srcRect.x + srcRect.dx; ++x)
        {
            if (rx < 0 ||
                rx >= static_cast<int32>(GetWidth()) ||
                ry < 0 ||
                ry >= static_cast<int32>(GetHeight()) ||
                x < 0 ||
                x >= static_cast<int32>(image->GetWidth()) ||
                y < 0 ||
                y >= static_cast<int32>(image->GetHeight()))
            {
                continue;
            }

            destData32[(rx) + (ry)*GetWidth()] = srcData32[x + y * image->GetWidth()];
            rx++;
        }
        ry++;
    }
}

void ImageExt::DrawImage(const SpriteBoundsRect& packedCell, const Rect2i& alphaOffsetRect, ImageExt* image)
{
    uint32* destData32 = reinterpret_cast<uint32*>(GetData());
    uint32* srcData32 = reinterpret_cast<uint32*>(image->GetData());
    const Rect2i& img = packedCell.spriteRect;

    bool withAlpha = CommandLineParser::Instance()->IsFlagSet("--disableCropAlpha");

    int32 sx = img.x;
    int32 sy = img.y;

    if (withAlpha)
    {
        sx += alphaOffsetRect.x;
        sy += alphaOffsetRect.y;
    }

    // add image
    const int32 imageHeight = static_cast<int32>(image->GetHeight());
    const int32 imageWidth = static_cast<int32>(image->GetWidth());
    int32 srcPos = 0;
    int32 destPos = sx + sy * GetWidth();
    int32 destPosInc = GetWidth() - imageWidth;
    for (int32 y = 0; y < imageHeight; ++y, destPos += destPosInc)
    {
        for (int32 x = 0; x < imageWidth; ++x, ++srcPos, ++destPos)
        {
            if ((sx + x) < 0)
                continue;
            if ((sx + x) >= static_cast<int32>(GetWidth()))
                continue;
            if ((sy + y) < 0)
                continue;
            if ((sy + y) >= static_cast<int32>(GetHeight()))
                continue;

            destData32[destPos] = srcData32[srcPos];
        }
    }

    uint32 x0 = img.x;
    uint32 xn = x0 + img.dx - 1;
    uint32 y0 = img.y;
    uint32 yn = y0 + img.dy - 1;
    uint32 yStride = GetWidth();

    if (packedCell.leftEdgePixel)
    {
        DVASSERT(x0 > 0);
        uint32 leftBorderPix = (x0 - 1) + (y0)*yStride;
        uint32 leftBorderLastPix = (x0 - 1) + (yn)*yStride;
        for (; leftBorderPix <= leftBorderLastPix; leftBorderPix += yStride)
        {
            destData32[leftBorderPix] = destData32[leftBorderPix + 1];
        }
    }

    if (packedCell.rightEdgePixel)
    {
        uint32 rightBorderPix = (xn + 1) + (y0)*yStride;
        uint32 rightBorderLastPix = (xn + 1) + (yn)*yStride;
        for (; rightBorderPix <= rightBorderLastPix; rightBorderPix += yStride)
        {
            destData32[rightBorderPix] = destData32[rightBorderPix - 1];
        }
    }

    if (packedCell.topEdgePixel)
    {
        DVASSERT(y0 > 0);
        uint32 topBorderPix = (x0) + (y0 - 1) * yStride;
        uint32 topImagePix = (x0) + (y0)*yStride;
        uint32 topBorderLastPix = (xn) + (y0 - 1) * yStride;
        for (; topBorderPix <= topBorderLastPix; ++topBorderPix, ++topImagePix)
        {
            destData32[topBorderPix] = destData32[topImagePix];
        }
    }

    if (packedCell.bottomEdgePixel)
    {
        uint32 bottomBorderPix = (x0) + (yn + 1) * yStride;
        uint32 bottomImagePix = (x0) + (yn)*yStride;
        uint32 bottomBorderLastPix = (xn) + (yn + 1) * yStride;
        for (; bottomBorderPix <= bottomBorderLastPix; ++bottomBorderPix, ++bottomImagePix)
        {
            destData32[bottomBorderPix] = destData32[bottomImagePix];
        }
    }

    if (packedCell.leftEdgePixel && packedCell.topEdgePixel)
        destData32[(x0 - 1) + (y0 - 1) * yStride] = destData32[(x0) + (y0)*yStride];

    if (packedCell.leftEdgePixel && packedCell.bottomEdgePixel)
        destData32[(x0 - 1) + (yn + 1) * yStride] = destData32[(x0) + (yn)*yStride];

    if (packedCell.rightEdgePixel && packedCell.topEdgePixel)
        destData32[(xn + 1) + (y0 - 1) * yStride] = destData32[(xn) + (y0)*yStride];

    if (packedCell.rightEdgePixel && packedCell.bottomEdgePixel)
        destData32[(xn + 1) + (yn + 1) * yStride] = destData32[(xn) + (yn)*yStride];
}

bool ImageExt::IsHorzLineOpaque(int32 y)
{
    uint8* line = GetData() + y * GetWidth() * 4;
    for (uint32 x = 0; x < GetWidth(); ++x)
    {
        if (line[x * 4 + 3] != 0)
        {
            return false;
        }
    }
    return true;
}

bool ImageExt::IsVertLineOpaque(int32 x)
{
    uint8* vertLine = GetData() + x * 4;
    for (uint32 i = 0; i < GetHeight(); ++i)
    {
        if (vertLine[3] != 0)
        {
            return false;
        }

        vertLine += GetWidth() * 4;
    }
    return true;
}

void ImageExt::FindNonOpaqueRect(Rect2i& rect)
{
    rect = Rect2i(0, 0, GetWidth(), GetHeight());
    for (uint32 y = 0; y < GetHeight(); ++y)
    {
        if (IsHorzLineOpaque(y))
        {
            rect.y++;
            rect.dy--;
        }
        else
        {
            break;
        }
    }

    for (uint32 x = 0; x < GetWidth(); ++x)
    {
        if (IsVertLineOpaque(x))
        {
            rect.x++;
            rect.dx--;
        }
        else
            break;
    }

    if ((rect.dx == 0) && (rect.dy == 0))
    {
        rect.x = rect.y = 0;
        rect.dx = rect.dy = 1;
        return;
    }

    for (int32 y = GetHeight() - 1; y >= 0; --y)
    {
        if (IsHorzLineOpaque(y))
        {
            rect.dy--;
        }
        else
        {
            break;
        }
    }

    for (int32 x = GetWidth() - 1; x >= 0; --x)
    {
        if (IsVertLineOpaque(x))
        {
            rect.dx--;
        }
        else
        {
            break;
        }
    }
}

void ImageExt::DrawRect(const Rect2i& rect, uint32 color)
{
    uint32* destData32 = reinterpret_cast<uint32*>(GetData());

    for (int32 i = 0; i < rect.dx; ++i)
    {
        destData32[rect.y * GetWidth() + rect.x + i] = color;
        destData32[(rect.y + rect.dy - 1) * GetWidth() + rect.x + i] = color;
    }
    for (int32 i = 0; i < rect.dy; ++i)
    {
        destData32[(rect.y + i) * GetWidth() + rect.x] = color;
        destData32[(rect.y + i) * GetWidth() + rect.x + rect.dx - 1] = color;
    }
}

void ImageExt::DitherAlpha()
{
    DVASSERT(internalData);

    if (internalData->format == FORMAT_RGBA8888)
    {
        ScopedPtr<Image> image(Image::Create(GetWidth(), GetHeight(), FORMAT_RGBA8888));

        uint8* ditheredPtr = image->GetData();
        uint8* dataPtr = GetData();

        for (uint32 y = 0; y < GetHeight(); ++y)
        {
            for (uint32 x = 0; x < GetWidth(); ++x)
            {
                if (dataPtr[3])
                {
                    Memcpy(ditheredPtr, dataPtr, 4);
                }
                else
                {
                    Color color = GetDitheredColorForPoint(x, y);

                    ditheredPtr[0] = static_cast<uint8>(color.r);
                    ditheredPtr[1] = static_cast<uint8>(color.g);
                    ditheredPtr[2] = static_cast<uint8>(color.b);
                    ditheredPtr[3] = 0;
                }

                ditheredPtr += 4;
                dataPtr += 4;
            }
        }

        internalData = image;
    }
}

Color ImageExt::GetDitheredColorForPoint(int32 x, int32 y)
{
    const uint8* imageData = GetData();

    int32 count = 0;
    Color newColor(0.0f, 0.0f, 0.0f, 0.0f);

    int32 startY = Max(y - 1, 0);
    int32 endY = Min(y + 1, static_cast<int32>(GetHeight()) - 1);
    int32 startX = Max(x - 1, 0);
    int32 endX = Min(x + 1, static_cast<int32>(GetWidth()) - 1);

    for (int32 alphaY = startY; alphaY <= endY; ++alphaY)
    {
        for (int32 alphaX = startX; alphaX <= endX; ++alphaX)
        {
            int32 offset = (alphaY * GetWidth() + alphaX) * 4;
            if (imageData[offset + 3])
            {
                ++count;
                newColor.r += static_cast<float32>(imageData[offset]);
                newColor.g += static_cast<float32>(imageData[offset + 1]);
                newColor.b += static_cast<float32>(imageData[offset + 2]);
            }
        }
    }

    if (count > 0)
    {
        newColor /= static_cast<float32>(count);
    }

    return newColor;
}
};
