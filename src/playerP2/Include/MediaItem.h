#pragma once

using namespace System;
using namespace System::IO;

namespace MyMediaPlayer {

    public ref class MediaItem {
    public:
        String^ FullPath;
        String^ FileName;

        MediaItem(String^ path) {
            FullPath = path;
            FileName = Path::GetFileName(path);
        }

        virtual String^ ToString() override {
            return FileName;
        }
    };
}