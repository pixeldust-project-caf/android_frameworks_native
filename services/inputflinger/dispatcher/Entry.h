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

#ifndef _UI_INPUT_INPUTDISPATCHER_ENTRY_H
#define _UI_INPUT_INPUTDISPATCHER_ENTRY_H

#include "InjectionState.h"
#include "InputTarget.h"

#include <gui/InputApplication.h>
#include <input/Input.h>
#include <stdint.h>
#include <utils/Timers.h>
#include <functional>
#include <string>

namespace android::inputdispatcher {

struct EventEntry {
    enum class Type {
        CONFIGURATION_CHANGED,
        DEVICE_RESET,
        FOCUS,
        KEY,
        MOTION,
        SENSOR,
        POINTER_CAPTURE_CHANGED,
        DRAG,
        TOUCH_MODE_CHANGED,

        ftl_last = TOUCH_MODE_CHANGED
    };

    int32_t id;
    Type type;
    nsecs_t eventTime;
    uint32_t policyFlags;
    InjectionState* injectionState;

    bool dispatchInProgress; // initially false, set to true while dispatching

    /**
     * Injected keys are events from an external (probably untrusted) application
     * and are not related to real hardware state. They come in via
     * InputDispatcher::injectInputEvent, which sets policy flag POLICY_FLAG_INJECTED.
     */
    inline bool isInjected() const { return injectionState != nullptr; }

    /**
     * Synthesized events are either injected events, or events that come
     * from real hardware, but aren't directly attributable to a specific hardware event.
     * Key repeat is a synthesized event, because it is related to an actual hardware state
     * (a key is currently pressed), but the repeat itself is generated by the framework.
     */
    inline bool isSynthesized() const {
        return isInjected() || IdGenerator::getSource(id) != IdGenerator::Source::INPUT_READER;
    }

    virtual std::string getDescription() const = 0;

    EventEntry(int32_t id, Type type, nsecs_t eventTime, uint32_t policyFlags);
    virtual ~EventEntry();

protected:
    void releaseInjectionState();
};

struct ConfigurationChangedEntry : EventEntry {
    explicit ConfigurationChangedEntry(int32_t id, nsecs_t eventTime);
    std::string getDescription() const override;

    ~ConfigurationChangedEntry() override;
};

struct DeviceResetEntry : EventEntry {
    int32_t deviceId;

    DeviceResetEntry(int32_t id, nsecs_t eventTime, int32_t deviceId);
    std::string getDescription() const override;

    ~DeviceResetEntry() override;
};

struct FocusEntry : EventEntry {
    sp<IBinder> connectionToken;
    bool hasFocus;
    std::string reason;

    FocusEntry(int32_t id, nsecs_t eventTime, sp<IBinder> connectionToken, bool hasFocus,
               const std::string& reason);
    std::string getDescription() const override;

    ~FocusEntry() override;
};

struct PointerCaptureChangedEntry : EventEntry {
    const PointerCaptureRequest pointerCaptureRequest;

    PointerCaptureChangedEntry(int32_t id, nsecs_t eventTime, const PointerCaptureRequest&);
    std::string getDescription() const override;

    ~PointerCaptureChangedEntry() override;
};

struct DragEntry : EventEntry {
    sp<IBinder> connectionToken;
    bool isExiting;
    float x, y;

    DragEntry(int32_t id, nsecs_t eventTime, sp<IBinder> connectionToken, bool isExiting, float x,
              float y);
    std::string getDescription() const override;

    ~DragEntry() override;
};

struct KeyEntry : EventEntry {
    int32_t deviceId;
    uint32_t source;
    int32_t displayId;
    int32_t action;
    int32_t flags;
    int32_t keyCode;
    int32_t scanCode;
    int32_t metaState;
    int32_t repeatCount;
    nsecs_t downTime;

    bool syntheticRepeat; // set to true for synthetic key repeats

    enum InterceptKeyResult {
        INTERCEPT_KEY_RESULT_UNKNOWN,
        INTERCEPT_KEY_RESULT_SKIP,
        INTERCEPT_KEY_RESULT_CONTINUE,
        INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER,
    };
    InterceptKeyResult interceptKeyResult; // set based on the interception result
    nsecs_t interceptKeyWakeupTime;        // used with INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER

    KeyEntry(int32_t id, nsecs_t eventTime, int32_t deviceId, uint32_t source, int32_t displayId,
             uint32_t policyFlags, int32_t action, int32_t flags, int32_t keyCode, int32_t scanCode,
             int32_t metaState, int32_t repeatCount, nsecs_t downTime);
    std::string getDescription() const override;
    void recycle();

    ~KeyEntry() override;
};

struct MotionEntry : EventEntry {
    int32_t deviceId;
    uint32_t source;
    int32_t displayId;
    int32_t action;
    int32_t actionButton;
    int32_t flags;
    int32_t metaState;
    int32_t buttonState;
    MotionClassification classification;
    int32_t edgeFlags;
    float xPrecision;
    float yPrecision;
    float xCursorPosition;
    float yCursorPosition;
    nsecs_t downTime;
    uint32_t pointerCount;
    PointerProperties pointerProperties[MAX_POINTERS];
    PointerCoords pointerCoords[MAX_POINTERS];

    MotionEntry(int32_t id, nsecs_t eventTime, int32_t deviceId, uint32_t source, int32_t displayId,
                uint32_t policyFlags, int32_t action, int32_t actionButton, int32_t flags,
                int32_t metaState, int32_t buttonState, MotionClassification classification,
                int32_t edgeFlags, float xPrecision, float yPrecision, float xCursorPosition,
                float yCursorPosition, nsecs_t downTime, uint32_t pointerCount,
                const PointerProperties* pointerProperties, const PointerCoords* pointerCoords);
    std::string getDescription() const override;

    ~MotionEntry() override;
};

struct SensorEntry : EventEntry {
    int32_t deviceId;
    uint32_t source;
    InputDeviceSensorType sensorType;
    InputDeviceSensorAccuracy accuracy;
    bool accuracyChanged;
    nsecs_t hwTimestamp;

    std::vector<float> values;

    SensorEntry(int32_t id, nsecs_t eventTime, int32_t deviceId, uint32_t source,
                uint32_t policyFlags, nsecs_t hwTimestamp, InputDeviceSensorType sensorType,
                InputDeviceSensorAccuracy accuracy, bool accuracyChanged,
                std::vector<float> values);
    std::string getDescription() const override;

    ~SensorEntry() override;
};

struct TouchModeEntry : EventEntry {
    bool inTouchMode;

    TouchModeEntry(int32_t id, nsecs_t eventTime, bool inTouchMode);
    std::string getDescription() const override;

    ~TouchModeEntry() override;
};

// Tracks the progress of dispatching a particular event to a particular connection.
struct DispatchEntry {
    const uint32_t seq; // unique sequence number, never 0

    std::shared_ptr<EventEntry> eventEntry; // the event to dispatch
    int32_t targetFlags;
    ui::Transform transform;
    ui::Transform rawTransform;
    float globalScaleFactor;
    // Both deliveryTime and timeoutTime are only populated when the entry is sent to the app,
    // and will be undefined before that.
    nsecs_t deliveryTime; // time when the event was actually delivered
    // An ANR will be triggered if a response for this entry is not received by timeoutTime
    nsecs_t timeoutTime;

    // Set to the resolved ID, action and flags when the event is enqueued.
    int32_t resolvedEventId;
    int32_t resolvedAction;
    int32_t resolvedFlags;

    DispatchEntry(std::shared_ptr<EventEntry> eventEntry, int32_t targetFlags,
                  const ui::Transform& transform, const ui::Transform& rawTransform,
                  float globalScaleFactor);

    inline bool hasForegroundTarget() const { return targetFlags & InputTarget::FLAG_FOREGROUND; }

    inline bool isSplit() const { return targetFlags & InputTarget::FLAG_SPLIT; }

private:
    static volatile int32_t sNextSeqAtomic;

    static uint32_t nextSeq();
};

VerifiedKeyEvent verifiedKeyEventFromKeyEntry(const KeyEntry& entry);
VerifiedMotionEvent verifiedMotionEventFromMotionEntry(const MotionEntry& entry,
                                                       const ui::Transform& rawTransform);

} // namespace android::inputdispatcher

#endif // _UI_INPUT_INPUTDISPATCHER_ENTRY_H
