#include "../include/ImagePackApi.h"
#include <stdio.h>
#include <vector>

static void PrintLastError(IMAGEPACK_HANDLE handle)
{
    IMAGEPACK_U32 required = 0;
    ImagePack_GetLastError(handle, NULL, 0, &required);
    std::vector<char> text(required ? required : 1, 0);
    ImagePack_GetLastError(handle, &text[0], (IMAGEPACK_U32)text.size(), NULL);
    printf("%s\n", &text[0]);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: ImagePackSdkExample <index.jph>\n");
        return 1;
    }

    printf("ImagePack SDK %s\n", ImagePack_GetApiVersionString());
    IMAGEPACK_HANDLE handle = ImagePack_Create();
    if (!handle) return 2;

    int r = ImagePack_OpenReader(handle, argv[1]);
    if (r != IMAGEPACK_OK)
    {
        printf("OpenReader failed: "); PrintLastError(handle);
        ImagePack_Destroy(handle);
        return 3;
    }

    IMAGEPACK_U64 count = 0;
    ImagePack_GetImageCount(handle, &count);
    printf("count=%llu\n", (unsigned long long)count);

    if (count)
    {
        IMAGEPACK_API_IMAGE_INFO info = { sizeof(info) };
        ImagePack_GetFirstImageInfo(handle, &info);
        printf("first: global=%llu source=%llu time=%llu jpeg=%u\n",
               (unsigned long long)info.globalIndex,
               (unsigned long long)info.sourceIndex,
               (unsigned long long)info.timeValue,
               info.jpegSize);

        IMAGEPACK_U32 size = 0;
        ImagePack_GetJpegSize(handle, 0, &size);
        std::vector<unsigned char> jpeg(size);
        IMAGEPACK_U32 actual = 0;
        r = ImagePack_ReadJpeg(handle, 0, jpeg.empty() ? NULL : &jpeg[0], size, &actual);
        printf("ReadJpeg ret=%d bytes=%u\n", r, actual);
    }

    ImagePack_Close(handle);
    ImagePack_Destroy(handle);
    return 0;
}
