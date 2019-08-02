/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERA_H
#define ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERA_H

#include <android/hardware/automotive/evs/1.1/types.h>
#include <android/hardware/automotive/evs/1.1/IEvsCamera.h>
#include <android/hardware/automotive/evs/1.1/IEvsCameraStream.h>
#include <ui/GraphicBuffer.h>

#include <thread>

using BufferDesc_1_0 = ::android::hardware::automotive::evs::V1_0::BufferDesc;
using BufferDesc_1_1 = ::android::hardware::automotive::evs::V1_1::BufferDesc;
using IEvsCameraStream_1_0 = ::android::hardware::automotive::evs::V1_0::IEvsCameraStream;
using IEvsCameraStream_1_1 = ::android::hardware::automotive::evs::V1_1::IEvsCameraStream;
using ::android::hardware::automotive::evs::V1_0::EvsResult;
using ::android::hardware::automotive::evs::V1_0::CameraDesc;


namespace android {
namespace hardware {
namespace automotive {
namespace evs {
namespace V1_1 {
namespace implementation {


// From EvsEnumerator.h
class EvsEnumerator;


class EvsCamera : public IEvsCamera {
public:
    // Methods from ::android::hardware::automotive::evs::V1_0::IEvsCamera follow.
    Return<void>      getCameraInfo(getCameraInfo_cb _hidl_cb)  override;
    Return<EvsResult> setMaxFramesInFlight(uint32_t bufferCount) override;
    Return<EvsResult> startVideoStream(const ::android::sp<IEvsCameraStream_1_0>& stream) override;
    Return<void>      stopVideoStream() override;
    Return<void>      doneWithFrame(const BufferDesc_1_0& buffer) override;

    Return<int32_t>   getExtendedInfo(uint32_t opaqueIdentifier) override;
    Return<EvsResult> setExtendedInfo(uint32_t opaqueIdentifier, int32_t opaqueValue) override;

    // Methods from ::android::hardware::automotive::evs::V1_1::IEvsCamera follow.
    Return<EvsResult> pauseVideoStream() override;
    Return<EvsResult> resumeVideoStream() override;
    Return<EvsResult> doneWithFrame_1_1(const BufferDesc_1_1& buffer) override;

    // Implementation details
    EvsCamera(const char *id);
    virtual ~EvsCamera() override;
    void forceShutdown();   // This gets called if another caller "steals" ownership of the camera

    const CameraDesc& getDesc() { return mDescription; };

    static const char kCameraName_Backup[];

private:
    // These three functions are expected to be called while mAccessLock is held
    bool setAvailableFrames_Locked(unsigned bufferCount);
    unsigned increaseAvailableFrames_Locked(unsigned numToAdd);
    unsigned decreaseAvailableFrames_Locked(unsigned numToRemove);

    void generateFrames();
    void fillTestFrame(const BufferDesc_1_0& buff);
    void fillTestFrame(const BufferDesc_1_1& buff);
    void returnBuffer(const uint32_t bufferId, const buffer_handle_t memHandle);

    sp<EvsEnumerator> mEnumerator;  // The enumerator object that created this camera

    CameraDesc mDescription = {};   // The properties of this camera

    std::thread mCaptureThread;     // The thread we'll use to synthesize frames

    uint32_t mWidth  = 0;           // Horizontal pixel count in the buffers
    uint32_t mHeight = 0;           // Vertical pixel count in the buffers
    uint32_t mFormat = 0;           // Values from android_pixel_format_t
    uint64_t mUsage  = 0;           // Values from from Gralloc.h
    uint32_t mStride = 0;           // Bytes per line in the buffers

    sp<IEvsCameraStream_1_1> mStream = nullptr;  // The callback used to deliver each frame

    struct BufferRecord {
        buffer_handle_t handle;
        bool inUse;

        explicit BufferRecord(buffer_handle_t h) : handle(h), inUse(false) {};
    };

    std::vector <BufferRecord> mBuffers;  // Graphics buffers to transfer images
    unsigned mFramesAllowed;              // How many buffers are we currently using
    unsigned mFramesInUse;                // How many buffers are currently outstanding

    enum StreamStateValues {
        STOPPED,
        RUNNING,
        STOPPING,
        DEAD,
    };
    StreamStateValues mStreamState;

    // Synchronization necessary to deconflict mCaptureThread from the main service thread
    std::mutex mAccessLock;
};

} // namespace implementation
} // namespace V1_1
} // namespace evs
} // namespace automotive
} // namespace hardware
} // namespace android

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERA_H