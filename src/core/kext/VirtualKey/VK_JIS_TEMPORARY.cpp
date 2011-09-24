#include "base.hpp"
#include "Config.hpp"
#include "EventOutputQueue.hpp"
#include "FlagStatus.hpp"
#include "VK_JIS_IM_CHANGE.hpp"
#include "VK_JIS_TEMPORARY.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  InputModeDetail VirtualKey::VK_JIS_TEMPORARY::savedinputmodedetail_(0);
  InputModeDetail VirtualKey::VK_JIS_TEMPORARY::currentinputmodedetail_(0);
  VirtualKey::VK_JIS_TEMPORARY::FireKeyInfo VirtualKey::VK_JIS_TEMPORARY::fireKeyInfo_;
  TimerWrapper VirtualKey::VK_JIS_TEMPORARY::fire_timer_;

  void
  VirtualKey::VK_JIS_TEMPORARY::initialize(IOWorkLoop& workloop)
  {
    fire_timer_.initialize(&workloop, NULL, VirtualKey::VK_JIS_TEMPORARY::fire_timer_callback);
    fireKeyInfo_.active = false;
  }

  void
  VirtualKey::VK_JIS_TEMPORARY::terminate(void)
  {
    fire_timer_.terminate();
  }

  bool
  VirtualKey::VK_JIS_TEMPORARY::handle_core(const Params_KeyboardEventCallBack& params,
                                            KeyCode key,
                                            InputModeDetail inputmodedetail)
  {
    if (params.key != key) return false;

    if (params.ex_iskeydown) {
      if (savedinputmodedetail_ == InputModeDetail::NONE) {
        Config::get_essential_config(BRIDGE_ESSENTIAL_CONFIG_INDEX_remap_jis_ignore_improvement_IM_changing) ?
        savedinputmodedetail_ = CommonData::getcurrent_workspacedata().inputmodedetail :
                                savedinputmodedetail_ = VirtualKey::VK_JIS_IM_CHANGE::getwsd_public().inputmodedetail;
        currentinputmodedetail_ = savedinputmodedetail_;
      }
      firekeytoinputdetail(params, inputmodedetail);
    }

    return true;
  }

  bool
  VirtualKey::VK_JIS_TEMPORARY::handle_RESTORE(const Params_KeyboardEventCallBack& params)
  {
    if (params.key != KeyCode::VK_JIS_TEMPORARY_RESTORE) return false;

    if (params.ex_iskeydown) {
      if (savedinputmodedetail_ != InputModeDetail::NONE) {
        if (! Config::get_essential_config(BRIDGE_ESSENTIAL_CONFIG_INDEX_remap_jis_ignore_improvement_IM_changing)) {
          VirtualKey::VK_JIS_IM_CHANGE::restore_WSD(savedinputmodedetail_);
          VirtualKey::VK_JIS_IM_CHANGE::set_omit_initialize(true);
        }

        firekeytoinputdetail(params, savedinputmodedetail_);
        savedinputmodedetail_ = InputModeDetail::NONE;
        currentinputmodedetail_ = InputModeDetail::NONE;
      }
    }

    return true;
  }

  bool
  VirtualKey::VK_JIS_TEMPORARY::handle(const Params_KeyboardEventCallBack& params)
  {
    // ------------------------------------------------------------
    if (handle_core(params,
                    KeyCode::VK_JIS_TEMPORARY_ROMAN,
                    InputModeDetail::ROMAN)) return true;

    if (handle_core(params,
                    KeyCode::VK_JIS_TEMPORARY_HIRAGANA,
                    InputModeDetail::JAPANESE_HIRAGANA)) return true;

    if (handle_core(params,
                    KeyCode::VK_JIS_TEMPORARY_KATAKANA,
                    InputModeDetail::JAPANESE_KATAKANA)) return true;

    // OK, Ainu is not Japanese.
    // But the input source of Ainu is Kotoeri, we need to handle it here.
    if (handle_core(params,
                    KeyCode::VK_JIS_TEMPORARY_AINU,
                    InputModeDetail::AINU)) return true;

    // ------------------------------------------------------------
    if (handle_RESTORE(params)) return true;

    // ------------------------------------------------------------
    // flash keyevent
    if (fireKeyInfo_.active) {
      fire_timer_.cancelTimeout();
      fire();
    }

    return false;
  }

  void
  VirtualKey::VK_JIS_TEMPORARY::firekeytoinputdetail(const Params_KeyboardEventCallBack& params,
                                                     InputModeDetail inputmodedetail)
  {
    inputmodedetail = normalize(inputmodedetail);
    currentinputmodedetail_ = normalize(currentinputmodedetail_);

    // ------------------------------------------------------------
    if (inputmodedetail == currentinputmodedetail_) return;
    if (inputmodedetail == InputModeDetail::NONE) return;

    currentinputmodedetail_ = inputmodedetail;

    // ------------------------------------------------------------
    if (inputmodedetail == InputModeDetail::ROMAN) {
      fireKeyInfo_.flags = Flags(0);
      fireKeyInfo_.key = KeyCode::JIS_EISUU;

    } else if (inputmodedetail == InputModeDetail::JAPANESE_HIRAGANA) {
      fireKeyInfo_.flags = Flags(0);
      fireKeyInfo_.key = KeyCode::JIS_KANA;

    } else if (inputmodedetail == InputModeDetail::JAPANESE_KATAKANA) {
      fireKeyInfo_.flags = ModifierFlag::SHIFT_L;
      fireKeyInfo_.key = KeyCode::JIS_KANA;

    } else if (inputmodedetail == InputModeDetail::JAPANESE_HALFWIDTH_KANA) {
      fireKeyInfo_.flags = ModifierFlag::CONTROL_L | ModifierFlag::SHIFT_L;
      fireKeyInfo_.key = KeyCode::SEMICOLON;

    } else if (inputmodedetail == InputModeDetail::JAPANESE_FULLWIDTH_ROMAN) {
      fireKeyInfo_.flags = ModifierFlag::CONTROL_L | ModifierFlag::SHIFT_L;
      fireKeyInfo_.key = KeyCode::L;

    } else if (inputmodedetail == InputModeDetail::AINU) {
      fireKeyInfo_.flags = ModifierFlag::OPTION_L;
      fireKeyInfo_.key = KeyCode::JIS_KANA;

    } else {
      return;
    }

    fireKeyInfo_.keyboardType = params.keyboardType;
    fireKeyInfo_.active = true;

    fire_timer_.setTimeoutMS(KEYEVENT_DELAY_MS);
  }

  InputModeDetail
  VirtualKey::VK_JIS_TEMPORARY::normalize(InputModeDetail imd)
  {
    if (imd == InputModeDetail::JAPANESE) {
      return InputModeDetail::JAPANESE_HIRAGANA;
    }
    return imd;
  }

  void
  VirtualKey::VK_JIS_TEMPORARY::fire_timer_callback(OSObject* notuse_owner, IOTimerEventSource* notuse_sender)
  {
    fire();
  }

  void
  VirtualKey::VK_JIS_TEMPORARY::fire(void)
  {
    if (! fireKeyInfo_.active) return;
    fireKeyInfo_.active = false;
    EventOutputQueue::FireKey::fire_downup(fireKeyInfo_.flags, fireKeyInfo_.key, fireKeyInfo_.keyboardType);
  }

  void
  VirtualKey::VK_JIS_TEMPORARY::resetSavedIMD(void)
  {
    savedinputmodedetail_   = InputModeDetail::NONE;
    currentinputmodedetail_ = InputModeDetail::NONE;
  }
}
