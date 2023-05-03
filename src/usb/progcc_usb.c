/*
 * Copyright (c) [2023] [Mitch Cairns/Handheldlegend, LLC]
 * All rights reserved.
 *
 * This source code is licensed under the provisions of the license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "progcc_usb.h"

progcc_usb_mode_t _progcc_usb_mode    = PUSB_MODE_XI;
progcc_usb_status_t _progcc_usb_status  = PUSB_STATUS_IDLE;
bool _progcc_usb_performance_mode = false;

typedef void (*usb_cb_t)(progcc_button_data_s *);

usb_cb_t _usb_hid_cb = NULL;

/* TEMPLATE FOR MODE SWITCH
switch(_progcc_usb_mode)
{
  case PUSB_MODE_MAX:
  default:
  case PUSB_MODE_NS:
    break;

  case PUSB_MODE_GC:
    break;

  case PUSB_MODE_XI:
    break;

  case PUSB_MODE_DI:
    break;
}
*/

void progcc_usb_set_mode(progcc_usb_mode_t mode, bool performance_mode)
{
  if (_progcc_usb_status != PUSB_STATUS_IDLE)
  {
    return;
  }

  _progcc_usb_performance_mode = performance_mode;

  switch(mode)
  {
    case PUSB_MODE_MAX:
    default:
    case PUSB_MODE_NS:
      _usb_hid_cb = nsinput_hid_report;
      break;

    case PUSB_MODE_GC:
      _usb_hid_cb = gcinput_hid_report;
      break;

    case PUSB_MODE_XI:
      _usb_hid_cb = xinput_hid_report;
      break;

    case PUSB_MODE_DI:
      _usb_hid_cb = dinput_hid_report;
      break;
  }

  _progcc_usb_mode = mode;
  _progcc_usb_status = PUSB_STATUS_INITIALIZED;
}

bool progcc_usb_start(void)
{
  if (_progcc_usb_status != PUSB_STATUS_INITIALIZED) return false;

  board_init();
  return tusb_init();
}

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {
  switch(_progcc_usb_mode)
  {
    case PUSB_MODE_MAX:
    default:
    case PUSB_MODE_NS:
      return (uint8_t const*) &ns_device_descriptor;
      break;

    case PUSB_MODE_GC:
      return (uint8_t const*) &gc_device_descriptor;
      break;

    case PUSB_MODE_XI:
      return (uint8_t const*) &xid_device_descriptor;
      break;

    case PUSB_MODE_DI:
      return (uint8_t const*) &di_device_descriptor;
      break;
  }
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;  // for multiple configurations
  switch(_progcc_usb_mode)
  {
    case PUSB_MODE_MAX:
    default:
    case PUSB_MODE_NS:
      if (_progcc_usb_performance_mode)
      {
        return (uint8_t const*) &ns_configuration_descriptor_performance;
      }
      return (uint8_t const*) &ns_configuration_descriptor;
      break;

    case PUSB_MODE_GC:
      if (_progcc_usb_performance_mode)
      {
        return (uint8_t const*) &gc_configuration_descriptor_performance;
      }
      return (uint8_t const*) &gc_configuration_descriptor;
      break;

    case PUSB_MODE_XI:
      return (uint8_t const*) &xid_configuration_descriptor;
      break;

    case PUSB_MODE_DI:
      return (uint8_t const*) &di_configuration_descriptor;
      break;
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  (void) instance;
  (void) report_id;
  (void) reqlen;

  return 0;
}

// Invoked when report complete
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
    switch (_progcc_usb_mode)
    {
        case PUSB_MODE_DI:
            if ((report[0] == 0x01) || (report[0] == 0x02))
            {
                //usb_process_data();
            }
            break;

        default:
        case PUSB_MODE_NS:
            if (len == NS_REPORT_LEN)
            {
                //usb_process_data();
            }
            break;
        case PUSB_MODE_XI:
            if ( (report[0] == 0x00) && (report[1] == XID_REPORT_LEN))
            {
                //usb_process_data();
            }

            break;

        case PUSB_MODE_GC:
            if (report[0] == 0x21)
            {
                //usb_process_data();
            }
            break;
    }

}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    switch (_progcc_usb_mode)
    {
        case PUSB_MODE_MAX:
        default:
        case PUSB_MODE_NS:
            break;
        case PUSB_MODE_DI:
            if (!report_id && !report_type)
            {
                /*
                if (buffer[0] == CMD_USB_REPORTID)
                {
                    //command_handler(buffer, bufsize);
                }*/
            }
            break;
        case PUSB_MODE_GC:
            if (!report_id && !report_type)
            {
                if (buffer[0] == 0x11)
                {
                    //rx_vibrate = (buffer[1] > 0) ? true : false;
                }
                else if (buffer[0] == 0x13)
                {
                    //ESP_LOGI("INIT", "Rx");
                }
            }
            break;
        case PUSB_MODE_XI:
            if (!report_id && !report_type)
            {
                if ((buffer[0] == 0x00) && (buffer[1] == 0x08))
                {
                    if ((buffer[3] > 0) || (buffer[4] > 0))
                    {
                        //rx_vibrate = 1;
                    }
                    else
                    {
                        //rx_vibrate = 0;
                    }
                }
            }
            break;
    }
}

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    switch (_progcc_usb_mode)
    {
        case PUSB_MODE_MAX:
        default:
        case PUSB_MODE_NS:
            return ns_hid_report_descriptor;
            break;
        case PUSB_MODE_GC:
            return gc_hid_report_descriptor;
            break;
        case PUSB_MODE_DI:
            return di_hid_report_descriptor;
            break;
    }
    return NULL;
}

// Set up custom TinyUSB XInput Driver
// Sets up custom TinyUSB Device Driver
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count += 1;
    return &tud_xinput_driver;
}

/********* USB Data Handling Utilities ***************/
/**
 * @brief Takes in directions and outputs a byte output appropriate for
 * HID Hat usage.
 * @param hat_type hat_mode_t type - The type of controller you're converting for.
 * @param leftRight 0 through 2 (2 is right) to indicate the direction left/right
 * @param upDown 0 through 2 (2 is up) to indicate the direction up/down
*/
uint8_t dir_to_hat(hat_mode_t hat_type, uint8_t leftRight, uint8_t upDown)
{
    uint8_t ret = 0;
    switch(hat_type)
    {
        default:
        case HAT_MODE_NS:
            ret = NS_HAT_CENTER;

        if (leftRight == 2)
        {
            ret = NS_HAT_RIGHT;
            if (upDown == 2)
            {
                ret = NS_HAT_TOP_RIGHT;
            }
            else if (upDown == 0)
            {
                ret = NS_HAT_BOTTOM_RIGHT;
            }
        }
        else if (leftRight == 0)
        {
            ret = NS_HAT_LEFT;
            if (upDown == 2)
            {
                ret = NS_HAT_TOP_LEFT;
            }
            else if (upDown == 0)
            {
                ret = NS_HAT_BOTTOM_LEFT;
            }
        }

        else if (upDown == 2)
        {
            ret = NS_HAT_TOP;
        }
        else if (upDown == 0)
        {
            ret = NS_HAT_BOTTOM;
        }

        return ret;
        break;

        case HAT_MODE_XI:
                ret = XI_HAT_CENTER;

            if (leftRight == 2)
            {
                ret = XI_HAT_RIGHT;
                if (upDown == 2)
                {
                    ret = XI_HAT_TOP_RIGHT;
                }
                else if (upDown == 0)
                {
                    ret = XI_HAT_BOTTOM_RIGHT;
                }
            }
            else if (leftRight == 0)
            {
                ret = XI_HAT_LEFT;
                if (upDown == 2)
                {
                    ret = XI_HAT_TOP_LEFT;
                }
                else if (upDown == 0)
                {
                    ret = XI_HAT_BOTTOM_LEFT;
                }
            }

            else if (upDown == 2)
            {
                ret = XI_HAT_TOP;
            }
            else if (upDown == 0)
            {
                ret = XI_HAT_BOTTOM;
            }

            return ret;
            break;
    }
}
