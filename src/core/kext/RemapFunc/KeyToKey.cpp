#include "CommonData.hpp"
#include "Config.hpp"
#include "EventOutputQueue.hpp"
#include "KeyToKey.hpp"
#include "KeyboardRepeat.hpp"
#include "VirtualKey.hpp"
#include "../VirtualKey/VK_JIS_IM_CHANGE.hpp"
#include "../VirtualKey/VK_JIS_TEMPORARY.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  namespace RemapFunc {
    KeyToKey::KeyToKey(void) : index_(0), keyboardRepeatID_(-1), isRepeatEnabled_(true)
    {
      toKeys_ = new Vector_PairKeyFlags();
    }

    KeyToKey::~KeyToKey(void)
    {
      if (toKeys_) {
        delete toKeys_;
      }
    }

    void
    KeyToKey::add(unsigned int datatype, unsigned int newval)
    {
      if (! toKeys_) return;

      switch (datatype) {
        case BRIDGE_DATATYPE_KEYCODE:
        {
          switch (index_) {
            case 0:
              fromKey_.key = newval;
              break;
            default:
              toKeys_->push_back(PairKeyFlags(newval));
              break;
          }
          ++index_;

          break;
        }

        case BRIDGE_DATATYPE_FLAGS:
        {
          switch (index_) {
            case 0:
              IOLOG_ERROR("Invalid KeyToKey::add\n");
              break;
            case 1:
              fromKey_.flags = newval;
              break;
            default:
              if (! toKeys_->empty()) {
                (toKeys_->back()).flags = newval;
              }
              break;
          }
          break;
        }

        case BRIDGE_DATATYPE_OPTION:
        {
          if (Option::NOREPEAT == newval) {
            isRepeatEnabled_ = false;
          } else {
            IOLOG_ERROR("KeyToKey::add unknown option:%d\n", newval);
          }
          break;
        }

        default:
          IOLOG_ERROR("KeyToKey::add invalid datatype:%d\n", datatype);
          break;
      }
    }

    bool
    KeyToKey::remap(RemapParams& remapParams)
    {
      if (! toKeys_) return false;

      int ignore_improveIM = Config::get_essential_config(BRIDGE_ESSENTIAL_CONFIG_INDEX_remap_jis_ignore_improvement_IM_changing);
      KeyCode firstKey      = (*toKeys_)[0].key;
      Flags firstKeyFlags = (*toKeys_)[0].flags;

      if (remapParams.isremapped) return false;
      if (! fromkeychecker_.isFromKey(remapParams.params.ex_iskeydown, remapParams.params.key, FlagStatus::makeFlags(), fromKey_.key, fromKey_.flags)) return false;
      remapParams.isremapped = true;

      if (toKeys_->size() > 0) {
        firstKey      = (*toKeys_)[0].key;
        firstKeyFlags = (*toKeys_)[0].flags;
      } else {
        firstKey      = KeyCode::VK_NONE;
      }

      // ------------------------------------------------------------
      // handle EventType & Modifiers

      // Let's consider the following setting.
      //   --KeyToKey-- KeyCode::SHIFT_R, ModifierFlag::SHIFT_R | ModifierFlag::NONE, KeyCode::A, ModifierFlag::SHIFT_R
      // In this setting, we need decrease SHIFT_R only once.
      // So, we transform values of fromKey_.
      //
      // [before]
      //   fromKey_.key   : KeyCode::SHIFT_R
      //   fromKey_.flags : ModifierFlag::SHIFT_R | ModifierFlag::NONE
      //
      // [after]
      //   fromKey_.key   : KeyCode::SHIFT_R
      //   fromKey_.flags : ModifierFlag::NONE
      //
      // Note: we need to apply this transformation after calling fromkeychecker_.isFromKey.

      Flags fromFlags = fromKey_.flags;
      fromFlags.remove(fromKey_.key.getModifierFlag());

      if (remapParams.params.ex_iskeydown) {
        FlagStatus::decrease(fromKey_.key.getModifierFlag());
      } else {
        FlagStatus::increase(fromKey_.key.getModifierFlag());
      }

      switch (toKeys_->size()) {
        case 0:
          break;

        case 1:
        {
          EventType newEventType = remapParams.params.ex_iskeydown ? EventType::DOWN : EventType::UP;
          KeyCode toKey = (*toKeys_)[0].key;
          ModifierFlag toModifierFlag = toKey.getModifierFlag();

          if (toModifierFlag == ModifierFlag::NONE && ! VirtualKey::isKeyLikeModifier(toKey)) {
            // toKey
            FlagStatus::temporary_decrease(fromFlags);
            FlagStatus::temporary_increase((*toKeys_)[0].flags);

          } else {
            // toModifier or VirtualKey::isKeyLikeModifier
            if (toModifierFlag != ModifierFlag::NONE) {
              newEventType = EventType::MODIFY;
            }

            if (remapParams.params.ex_iskeydown) {
              FlagStatus::increase((*toKeys_)[0].flags | toModifierFlag);
              FlagStatus::decrease(fromFlags);
            } else {
              FlagStatus::decrease((*toKeys_)[0].flags | toModifierFlag);
              FlagStatus::increase(fromFlags);
            }
          }

          // ----------------------------------------
          Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(newEventType,
                                                                                         FlagStatus::makeFlags(),
                                                                                         toKey,
                                                                                         remapParams.params.keyboardType,
                                                                                         remapParams.params.repeat));
          if (! ptr) return false;
          Params_KeyboardEventCallBack& params = *ptr;

          if (remapParams.params.ex_iskeydown) {
            if (firstKey == KeyCode::JIS_KANA || firstKey == KeyCode::JIS_EISUU) {
              if (ignore_improveIM) {
                VirtualKey::VK_JIS_IM_CHANGE::set_omit_initialize(false);
              } else {
                VirtualKey::VK_JIS_IM_CHANGE::replace_WSD(firstKey, firstKeyFlags);
                VirtualKey::VK_JIS_IM_CHANGE::set_omit_initialize(true);
              }
            } else if (firstKey != KeyCode::VK_JIS_TEMPORARY_KATAKANA &&
                       firstKey != KeyCode::VK_JIS_TEMPORARY_HIRAGANA &&
                       firstKey != KeyCode::VK_JIS_TEMPORARY_ROMAN    &&
                       firstKey != KeyCode::VK_JIS_TEMPORARY_AINU     &&
                       firstKey != KeyCode::VK_JIS_TEMPORARY_RESTORE  &&
                       ! VirtualKey::VK_JIS_IM_CHANGE::get_case1_pass_restore() &&
                       ! ignore_improveIM) {
              EventOutputQueue::FireKey::fire_downup(Flags(0), KeyCode::VK_JIS_TEMPORARY_RESTORE,
                                                     remapParams.params.keyboardType);
            }

            if (! ignore_improveIM) {
              VirtualKey::VK_JIS_IM_CHANGE::cancelCallback();
              VirtualKey::VK_JIS_IM_CHANGE::set_omit_initialize(true);
            }
          }

          if (remapParams.params.ex_iskeydown && ! isRepeatEnabled_) {
            KeyboardRepeat::cancel();
          } else {
            KeyboardRepeat::set(params);
          }
          EventOutputQueue::FireKey::fire(params);

          break;
        }

        default:
          KeyCode lastKey                  = (*toKeys_)[toKeys_->size() - 1].key;
          Flags lastKeyFlags               = (*toKeys_)[toKeys_->size() - 1].flags;
          ModifierFlag lastKeyModifierFlag = lastKey.getModifierFlag();
          bool isLastKeyModifier           = (lastKeyModifierFlag != ModifierFlag::NONE);
          bool isLastKeyLikeModifier       = VirtualKey::isKeyLikeModifier(lastKey);

          if (remapParams.params.ex_iskeydown) {
            if (firstKey != KeyCode::VK_JIS_TEMPORARY_KATAKANA &&
                firstKey != KeyCode::VK_JIS_TEMPORARY_HIRAGANA &&
                firstKey != KeyCode::VK_JIS_TEMPORARY_ROMAN    &&
                firstKey != KeyCode::VK_JIS_TEMPORARY_AINU     &&
                firstKey != KeyCode::VK_JIS_TEMPORARY_RESTORE  &&
                ! ignore_improveIM) {
              EventOutputQueue::FireKey::fire_downup(Flags(0), KeyCode::VK_JIS_TEMPORARY_RESTORE,
                                                     remapParams.params.keyboardType);
            }

            KeyboardRepeat::cancel();

            FlagStatus::temporary_decrease(fromFlags);

            size_t size = toKeys_->size();
            // If the last key is modifier, we give it special treatment.
            // - Don't fire key repeat.
            // - Synchronous the key press status and the last modifier status.
            if (isLastKeyModifier || isLastKeyLikeModifier) {
              --size;
            }

            for (size_t i = 0; i < size; ++i) {
              FlagStatus::temporary_increase((*toKeys_)[i].flags);

              Flags f = FlagStatus::makeFlags();
              KeyboardType keyboardType = remapParams.params.keyboardType;

              if (i == size - 1 && (*toKeys_)[i].key == KeyCode::VK_JIS_TEMPORARY_RESTORE && ! ignore_improveIM) {
                break;
              } else if ( (*toKeys_)[i].key == KeyCode::JIS_KANA || (*toKeys_)[i].key == KeyCode::JIS_EISUU) {
                if (ignore_improveIM) {
                  VirtualKey::VK_JIS_IM_CHANGE::set_omit_initialize(false);
                } else {
                  VirtualKey::VK_JIS_IM_CHANGE::replace_WSD((*toKeys_)[i].key, (*toKeys_)[i].flags);
                  VirtualKey::VK_JIS_IM_CHANGE::set_omit_initialize(true);
                }
              }

              EventOutputQueue::FireKey::fire_downup(f, (*toKeys_)[i].key, keyboardType);
              KeyboardRepeat::primitive_add_downup(f, (*toKeys_)[i].key, keyboardType);

              FlagStatus::temporary_decrease((*toKeys_)[i].flags);
            }

            if (isLastKeyModifier || isLastKeyLikeModifier) {
              // restore temporary flag.
              FlagStatus::temporary_increase(fromFlags);

              FlagStatus::increase(lastKeyFlags | lastKeyModifierFlag);
              FlagStatus::decrease(fromFlags);

              if (isLastKeyLikeModifier) {
                // Don't call EventOutputQueue::FireModifiers::fire here.
                //
                // Intentionally VK_LAZY_* stop sending MODIFY events.
                // EventOutputQueue::FireModifiers::fire destroys this behavior.
                Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(EventType::DOWN, FlagStatus::makeFlags(), lastKey, remapParams.params.keyboardType, false));
                if (ptr) {
                  EventOutputQueue::FireKey::fire(*ptr);
                }
              } else {
                EventOutputQueue::FireModifiers::fire();
              }
            }

            if (isLastKeyModifier || isLastKeyLikeModifier) {
              KeyboardRepeat::cancel();
            } else {
              if (isRepeatEnabled_) {
                keyboardRepeatID_ = KeyboardRepeat::primitive_start();
              } else {
                keyboardRepeatID_ = -1;
              }
            }

          } else {
            if (! ignore_improveIM && lastKey == KeyCode::VK_JIS_TEMPORARY_RESTORE) {
              VirtualKey::VK_JIS_IM_CHANGE::scheduleCallback(VirtualKey::VK_JIS_IM_CHANGE::CallbackType::RESTORE);
            } else if (! ignore_improveIM) {
              VirtualKey::VK_JIS_IM_CHANGE::cancelCallback();
            }

            if (isLastKeyModifier || isLastKeyLikeModifier) {
              // For Lazy-Modifiers (KeyCode::VK_LAZY_*),
              // we need to handle these keys before restoring fromFlags, lastKeyFlags and lastKeyModifierFlag.
              // The unnecessary modifier events occur unless we do it.
              if (isLastKeyLikeModifier) {
                Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(EventType::UP, FlagStatus::makeFlags(), lastKey, remapParams.params.keyboardType, false));
                if (ptr) {
                  EventOutputQueue::FireKey::fire(*ptr);
                }
              }

              FlagStatus::decrease(lastKeyFlags | lastKeyModifierFlag);
              FlagStatus::increase(fromFlags);
              EventOutputQueue::FireModifiers::fire();

            } else {
              if (KeyboardRepeat::getID() == keyboardRepeatID_) {
                KeyboardRepeat::cancel();
              }
            }
          }
          break;
      }

      return true;
    }

    bool
    KeyToKey::call_remap_with_VK_PSEUDO_KEY(EventType eventType)
    {
      Params_KeyboardEventCallBack::auto_ptr ptr(Params_KeyboardEventCallBack::alloc(eventType,
                                                                                     FlagStatus::makeFlags(),
                                                                                     KeyCode::VK_PSEUDO_KEY,
                                                                                     CommonData::getcurrent_keyboardType(),
                                                                                     false));
      if (! ptr) return false;
      Params_KeyboardEventCallBack& params = *ptr;

      RemapParams rp(params);
      return remap(rp);
    }
  }
}
