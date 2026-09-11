/*
 * CH585 全速USB设备驱动（USB1，左边type C）
 * 标准键盘+鼠标复合HID设备
 * 基于官方HID_CompliantDev示例，描述符参考高速口CompositeKM
 */

#include "CH58x_common.h"
#include "ch585_usbfs_device.h"

#define DevEP0SIZE    0x40
#define DevEP1SIZE    0x08
#define DevEP2SIZE    0x08

// 设备描述符
const uint8_t FS_MyDevDescr[] = {
    0x12,                                                   // bLength
    0x01,                                                   // bDescriptorType
    0x00, 0x02,                                             // bcdUSB
    0x00,                                                   // bDeviceClass
    0x00,                                                   // bDeviceSubClass
    0x00,                                                   // bDeviceProtocol
    DevEP0SIZE,                                             // bMaxPacketSize0
    0x3d, 0x41,                                             // idVendor
    0x07, 0x21,                                             // idProduct
    0x00, 0x01,                                             // bcdDevice
    0x01,                                                   // iManufacturer
    0x02,                                                   // iProduct
    0x03,                                                   // iSerialNumber
    0x01,                                                   // bNumConfigurations
};

// 配置描述符（键盘+鼠标+Raw HID复合设备）
const uint8_t FS_MyCfgDescr[] = {
    /* Configuration Descriptor */
    0x09,                                                   // bLength
    0x02,                                                   // bDescriptorType
    0x5B, 0x00,                                             // wTotalLength (91 bytes)
    0x03,                                                   // bNumInterfaces
    0x01,                                                   // bConfigurationValue
    0x00,                                                   // iConfiguration
    0xA0,                                                   // bmAttributes: Bus Powered; Remote Wakeup
    0x32,                                                   // MaxPower: 100mA

    /* Interface Descriptor (Keyboard) */
    0x09,                                                   // bLength
    0x04,                                                   // bDescriptorType
    0x00,                                                   // bInterfaceNumber
    0x00,                                                   // bAlternateSetting
    0x01,                                                   // bNumEndpoints
    0x03,                                                   // bInterfaceClass
    0x01,                                                   // bInterfaceSubClass
    0x01,                                                   // bInterfaceProtocol: Keyboard
    0x00,                                                   // iInterface

    /* HID Descriptor (Keyboard) */
    0x09,                                                   // bLength
    0x21,                                                   // bDescriptorType
    0x11, 0x01,                                             // bcdHID
    0x00,                                                   // bCountryCode
    0x01,                                                   // bNumDescriptors
    0x22,                                                   // bDescriptorType
    0x3E, 0x00,                                             // wDescriptorLength

    /* Endpoint Descriptor (Keyboard) */
    0x07,                                                   // bLength
    0x05,                                                   // bDescriptorType
    0x81,                                                   // bEndpointAddress: IN Endpoint 1
    0x03,                                                   // bmAttributes
    DevEP1SIZE, 0x00,                                       // wMaxPacketSize
    0x0A,                                                   // bInterval: 10mS

    /* Interface Descriptor (Mouse) */
    0x09,                                                   // bLength
    0x04,                                                   // bDescriptorType
    0x01,                                                   // bInterfaceNumber
    0x00,                                                   // bAlternateSetting
    0x01,                                                   // bNumEndpoints
    0x03,                                                   // bInterfaceClass
    0x01,                                                   // bInterfaceSubClass
    0x02,                                                   // bInterfaceProtocol: Mouse
    0x00,                                                   // iInterface

    /* HID Descriptor (Mouse) */
    0x09,                                                   // bLength
    0x21,                                                   // bDescriptorType
    0x10, 0x01,                                             // bcdHID
    0x00,                                                   // bCountryCode
    0x01,                                                   // bNumDescriptors
    0x22,                                                   // bDescriptorType
    0x34, 0x00,                                             // wDescriptorLength

    /* Endpoint Descriptor (Mouse) */
    0x07,                                                   // bLength
    0x05,                                                   // bDescriptorType
    0x82,                                                   // bEndpointAddress: IN Endpoint 2
    0x03,                                                   // bmAttributes
    DevEP2SIZE, 0x00,                                       // wMaxPacketSize
    0x01,                                                   // bInterval: 1mS

    /* Interface Descriptor (Raw HID) */
    0x09,                                                   // bLength
    0x04,                                                   // bDescriptorType
    0x02,                                                   // bInterfaceNumber
    0x00,                                                   // bAlternateSetting
    0x02,                                                   // bNumEndpoints
    0x03,                                                   // bInterfaceClass: HID
    0x00,                                                   // bInterfaceSubClass
    0x00,                                                   // bInterfaceProtocol
    0x00,                                                   // iInterface

    /* HID Descriptor (Raw HID) */
    0x09,                                                   // bLength
    0x21,                                                   // bDescriptorType
    0x11, 0x01,                                             // bcdHID
    0x00,                                                   // bCountryCode
    0x01,                                                   // bNumDescriptors
    0x22,                                                   // bDescriptorType
    0x19, 0x00,                                             // wDescriptorLength (25 bytes)

    /* Endpoint Descriptor (Raw HID IN) */
    0x07,                                                   // bLength
    0x05,                                                   // bDescriptorType
    0x83,                                                   // bEndpointAddress: IN Endpoint 3
    0x03,                                                   // bmAttributes: Interrupt
    0x40, 0x00,                                             // wMaxPacketSize: 64 bytes
    0x01,                                                   // bInterval: 1mS

    /* Endpoint Descriptor (Raw HID OUT) */
    0x07,                                                   // bLength
    0x05,                                                   // bDescriptorType
    0x03,                                                   // bEndpointAddress: OUT Endpoint 3
    0x03,                                                   // bmAttributes: Interrupt
    0x40, 0x00,                                             // wMaxPacketSize: 64 bytes
    0x01                                                    // bInterval: 1mS
};

// 键盘报告描述符
const uint8_t FS_KeyRepDesc[] = {
    0x05, 0x01,                                             // Usage Page (Generic Desktop)
    0x09, 0x06,                                             // Usage (Keyboard)
    0xA1, 0x01,                                             // Collection (Application)
    0x05, 0x07,                                             // Usage Page (Key Codes)
    0x19, 0xE0,                                             // Usage Minimum (224)
    0x29, 0xE7,                                             // Usage Maximum (231)
    0x15, 0x00,                                             // Logical Minimum (0)
    0x25, 0x01,                                             // Logical Maximum (1)
    0x75, 0x01,                                             // Report Size (1)
    0x95, 0x08,                                             // Report Count (8)
    0x81, 0x02,                                             // Input (Data,Variable,Absolute)
    0x95, 0x01,                                             // Report Count (1)
    0x75, 0x08,                                             // Report Size (8)
    0x81, 0x01,                                             // Input (Constant)
    0x95, 0x03,                                             // Report Count (3)
    0x75, 0x01,                                             // Report Size (1)
    0x05, 0x08,                                             // Usage Page (LEDs)
    0x19, 0x01,                                             // Usage Minimum (1)
    0x29, 0x03,                                             // Usage Maximum (3)
    0x91, 0x02,                                             // Output (Data,Variable,Absolute)
    0x95, 0x05,                                             // Report Count (5)
    0x75, 0x01,                                             // Report Size (1)
    0x91, 0x01,                                             // Output (Constant,Array,Absolute)
    0x95, 0x06,                                             // Report Count (6)
    0x75, 0x08,                                             // Report Size (8)
    0x26, 0xFF, 0x00,                                       // Logical Maximum (255)
    0x05, 0x07,                                             // Usage Page (Key Codes)
    0x19, 0x00,                                             // Usage Minimum (0)
    0x29, 0x91,                                             // Usage Maximum (145)
    0x81, 0x00,                                             // Input(Data,Array,Absolute)
    0xC0                                                    // End Collection
};

// 鼠标报告描述符
const uint8_t FS_MouseRepDesc[] = {
    0x05, 0x01,                                             // Usage Page (Generic Desktop)
    0x09, 0x02,                                             // Usage (Mouse)
    0xA1, 0x01,                                             // Collection (Application)
    0x09, 0x01,                                             // Usage (Pointer)
    0xA1, 0x00,                                             // Collection (Physical)
    0x05, 0x09,                                             // Usage Page (Button)
    0x19, 0x01,                                             // Usage Minimum (Button 1)
    0x29, 0x03,                                             // Usage Maximum (Button 3)
    0x15, 0x00,                                             // Logical Minimum (0)
    0x25, 0x01,                                             // Logical Maximum (1)
    0x75, 0x01,                                             // Report Size (1)
    0x95, 0x03,                                             // Report Count (3)
    0x81, 0x02,                                             // Input (Data,Variable,Absolute)
    0x75, 0x05,                                             // Report Size (5)
    0x95, 0x01,                                             // Report Count (1)
    0x81, 0x01,                                             // Input (Constant,Array,Absolute)
    0x05, 0x01,                                             // Usage Page (Generic Desktop)
    0x09, 0x30,                                             // Usage (X)
    0x09, 0x31,                                             // Usage (Y)
    0x09, 0x38,                                             // Usage (Wheel)
    0x15, 0x81,                                             // Logical Minimum (-127)
    0x25, 0x7F,                                             // Logical Maximum (127)
    0x75, 0x08,                                             // Report Size (8)
    0x95, 0x03,                                             // Report Count (3)
    0x81, 0x06,                                             // Input (Data,Variable,Relative)
    0xC0,                                                   // End Collection
    0xC0                                                    // End Collection
};

// Raw HID报告描述符（QMK/Vial兼容，25字节）
const uint8_t FS_RawHIDRepDesc[] = {
    0x06, 0x60, 0xFF,                                       // Usage Page (Vendor Defined 0xFF60)
    0x09, 0x61,                                             // Usage (0x61)
    0xA1, 0x01,                                             // Collection (Application)
    0x09, 0x62,                                             //   Usage (0x62)
    0x15, 0x00,                                             //   Logical Minimum (0)
    0x26, 0xFF, 0x00,                                       //   Logical Maximum (255)
    0x75, 0x08,                                             //   Report Size (8)
    0x95, 0x20,                                             //   Report Count (32)
    0x81, 0x02,                                             //   Input (Data,Variable,Absolute)
    0x09, 0x63,                                             //   Usage (0x63)
    0x91, 0x02,                                             //   Output (Data,Variable,Absolute)
    0xC0                                                     // End Collection
};

// 语言描述符
const uint8_t FS_MyLangDescr[] = {
    0x04, 0x03, 0x09, 0x04
};

// 厂商描述符
const uint8_t FS_MyManuInfo[] = {
    0x0E, 0x03,
    'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0
};

// 产品描述符
const uint8_t FS_MyProdInfo[] = {
    0x0C, 0x03,
    'C', 0, 'H', 0, '5', 0, '8', 0, '5', 0
};

// 序列号描述符
const uint8_t FS_MySerNumInfo[] = {
    0x16, 0x03,
    '0', 0, '1', 0, '2', 0, '3', 0, '4', 0,
    '5', 0, '6', 0, '7', 0, '8', 0, '9', 0
};

// 全局变量
uint8_t        DevConfig, Ready = 0;
uint8_t        SetupReqCode;
uint16_t       SetupReqLen;
const uint8_t *pDescr;
uint8_t        Report_Value = 0x00;
uint8_t        Idle_Value = 0x00;
uint8_t        USB_SleepStatus = 0x00;

// 全速USB枚举状态
volatile uint8_t USBFS_DevEnumStatus = 0;

// 端点DMA缓冲区（4字节对齐）
__attribute__((aligned(4))) uint8_t EP0_Databuf[64 + 64 + 64];
__attribute__((aligned(4))) uint8_t EP1_Databuf[64 + 64];
__attribute__((aligned(4))) uint8_t EP2_Databuf[64 + 64];
__attribute__((aligned(4))) uint8_t EP3_Databuf[64 + 64];

// USB设备传输处理
void USB_DevTransProcess(void)
{
    uint8_t len, chtype;
    uint8_t intflag, errflag = 0;

    intflag = R8_USB_INT_FG;

    if(intflag & RB_UIF_TRANSFER)
    {
        if((R8_USB_INT_ST & MASK_UIS_TOKEN) != MASK_UIS_TOKEN)
        {
            switch(R8_USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
            {
                case UIS_TOKEN_IN:
                {
                    switch(SetupReqCode)
                    {
                        case USB_GET_DESCRIPTOR:
                            len = SetupReqLen >= DevEP0SIZE ? DevEP0SIZE : SetupReqLen;
                            memcpy(pEP0_DataBuf, pDescr, len);
                            SetupReqLen -= len;
                            pDescr += len;
                            R8_UEP0_T_LEN = len;
                            R8_UEP0_CTRL ^= RB_UEP_T_TOG;
                            break;

                        case USB_SET_ADDRESS:
                            R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | SetupReqLen;
                            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                            break;

                        case USB_SET_FEATURE:
                            break;

                        default:
                            R8_UEP0_T_LEN = 0;
                            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                            Ready = 1;
                            USBFS_DevEnumStatus = 1;
                            break;
                    }
                }
                break;

                case UIS_TOKEN_OUT:
                {
                    len = R8_USB_RX_LEN;
                }
                break;

                case UIS_TOKEN_OUT | 1:
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)
                    {
                        R8_UEP1_CTRL ^= RB_UEP_R_TOG;
                        len = R8_USB_RX_LEN;
                    }
                }
                break;

                case UIS_TOKEN_IN | 1:
                    R8_UEP1_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    Ready = 1;
                    break;

                case UIS_TOKEN_IN | 2:
                    R8_UEP2_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    break;

                case UIS_TOKEN_OUT | 3:
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)
                    {
                        R8_UEP3_CTRL ^= RB_UEP_R_TOG;
                        len = R8_USB_RX_LEN;
                        if(len > 0)
                        {
                            extern void raw_hid_receive(uint8_t *data, uint8_t length);
                            raw_hid_receive(pEP3_OUT_DataBuf, len);
                        }
                    }
                }
                break;

                case UIS_TOKEN_IN | 3:
                    R8_UEP3_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    break;
            }
            R8_USB_INT_FG = RB_UIF_TRANSFER;
        }

        if(R8_USB_INT_ST & RB_UIS_SETUP_ACT)
        {
            R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
            SetupReqLen = pSetupReqPak->wLength;
            SetupReqCode = pSetupReqPak->bRequest;
            chtype = pSetupReqPak->bRequestType;

            len = 0;
            errflag = 0;
            if((pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
            {
                if(pSetupReqPak->bRequestType & 0x40)
                {
                }
                else if(pSetupReqPak->bRequestType & 0x20)
                {
                    switch(SetupReqCode)
                    {
                        case DEF_USB_SET_IDLE:
                            Idle_Value = EP0_Databuf[3];
                            break;

                        case DEF_USB_SET_REPORT:
                            break;

                        case DEF_USB_SET_PROTOCOL:
                            Report_Value = EP0_Databuf[2];
                            break;

                        case DEF_USB_GET_IDLE:
                            EP0_Databuf[0] = Idle_Value;
                            len = 1;
                            break;

                        case DEF_USB_GET_PROTOCOL:
                            EP0_Databuf[0] = Report_Value;
                            len = 1;
                            break;

                        default:
                            errflag = 0xFF;
                    }
                }
            }
            else
            {
                switch(SetupReqCode)
                {
                    case USB_GET_DESCRIPTOR:
                    {
                        switch(((pSetupReqPak->wValue) >> 8))
                        {
                            case USB_DESCR_TYP_DEVICE:
                            {
                                pDescr = FS_MyDevDescr;
                                len = FS_MyDevDescr[0];
                            }
                            break;

                            case USB_DESCR_TYP_CONFIG:
                            {
                                pDescr = FS_MyCfgDescr;
                                len = FS_MyCfgDescr[2];
                            }
                            break;

                            case USB_DESCR_TYP_HID:
                                switch((pSetupReqPak->wIndex) & 0xff)
                                {
                                    case 0:
                                        pDescr = (uint8_t *)(&FS_MyCfgDescr[18]);
                                        len = 9;
                                        break;
                                    case 1:
                                        pDescr = (uint8_t *)(&FS_MyCfgDescr[43]);
                                        len = 9;
                                        break;
                                    case 2:
                                        pDescr = (uint8_t *)(&FS_MyCfgDescr[68]);
                                        len = 9;
                                        break;
                                    default:
                                        errflag = 0xff;
                                        break;
                                }
                                break;

                            case USB_DESCR_TYP_REPORT:
                            {
                                if(((pSetupReqPak->wIndex) & 0xff) == 0)
                                {
                                    pDescr = FS_KeyRepDesc;
                                    len = sizeof(FS_KeyRepDesc);
                                }
                                else if(((pSetupReqPak->wIndex) & 0xff) == 1)
                                {
                                    pDescr = FS_MouseRepDesc;
                                    len = sizeof(FS_MouseRepDesc);
                                }
                                else if(((pSetupReqPak->wIndex) & 0xff) == 2)
                                {
                                    pDescr = FS_RawHIDRepDesc;
                                    len = sizeof(FS_RawHIDRepDesc);
                                }
                                else
                                    errflag = 0xff;
                            }
                            break;

                            case USB_DESCR_TYP_STRING:
                            {
                                switch((pSetupReqPak->wValue) & 0xff)
                                {
                                    case 0:
                                        pDescr = FS_MyLangDescr;
                                        len = sizeof(FS_MyLangDescr);
                                        break;
                                    case 1:
                                        pDescr = FS_MyManuInfo;
                                        len = sizeof(FS_MyManuInfo);
                                        break;
                                    case 2:
                                        pDescr = FS_MyProdInfo;
                                        len = sizeof(FS_MyProdInfo);
                                        break;
                                    case 3:
                                        pDescr = FS_MySerNumInfo;
                                        len = sizeof(FS_MySerNumInfo);
                                        break;
                                    default:
                                        errflag = 0xFF;
                                        break;
                                }
                            }
                            break;

                            default:
                                errflag = 0xff;
                                break;
                        }
                        if(SetupReqLen > len)
                            SetupReqLen = len;
                        len = (SetupReqLen >= DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
                        memcpy(pEP0_DataBuf, pDescr, len);
                        pDescr += len;
                    }
                    break;

                    case USB_SET_ADDRESS:
                        SetupReqLen = (pSetupReqPak->wValue) & 0xff;
                        break;

                    case USB_GET_CONFIGURATION:
                        pEP0_DataBuf[0] = DevConfig;
                        if(SetupReqLen > 1)
                            SetupReqLen = 1;
                        break;

                    case USB_SET_CONFIGURATION:
                        DevConfig = (pSetupReqPak->wValue) & 0xff;
                        break;

                    case USB_CLEAR_FEATURE:
                    {
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                        {
                            switch((pSetupReqPak->wIndex) & 0xff)
                            {
                                case 0x81:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                    break;
                                case 0x82:
                                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                    break;
                                case 0x01:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                                    break;
                                default:
                                    errflag = 0xFF;
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                        {
                            if(pSetupReqPak->wValue == 1)
                            {
                                USB_SleepStatus &= ~0x01;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                    }
                    break;

                    case USB_SET_FEATURE:
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                        {
                            switch(pSetupReqPak->wIndex)
                            {
                                case 0x81:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                                    break;
                                case 0x82:
                                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                                    break;
                                case 0x01:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                                    break;
                                default:
                                    errflag = 0xFF;
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                        {
                            if(pSetupReqPak->wValue == 1)
                            {
                                USB_SleepStatus |= 0x01;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                        break;

                    case USB_GET_INTERFACE:
                        pEP0_DataBuf[0] = 0x00;
                        if(SetupReqLen > 1)
                            SetupReqLen = 1;
                        break;

                    case USB_SET_INTERFACE:
                        break;

                    case USB_GET_STATUS:
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                        {
                            pEP0_DataBuf[0] = 0x00;
                            switch(pSetupReqPak->wIndex)
                            {
                                case 0x81:
                                    if((R8_UEP1_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;
                                case 0x82:
                                    if((R8_UEP2_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;
                                case 0x01:
                                    if((R8_UEP1_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                        {
                            pEP0_DataBuf[0] = 0x00;
                            if(USB_SleepStatus)
                            {
                                pEP0_DataBuf[0] = 0x02;
                            }
                            else
                            {
                                pEP0_DataBuf[0] = 0x00;
                            }
                        }
                        pEP0_DataBuf[1] = 0;
                        if(SetupReqLen >= 2)
                        {
                            SetupReqLen = 2;
                        }
                        break;

                    default:
                        errflag = 0xff;
                        break;
                }
            }
            if(errflag == 0xff)
            {
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
                Ready = 1;
            }
            else
            {
                if(chtype & 0x80)
                {
                    len = (SetupReqLen > DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
                    SetupReqLen -= len;
                }
                else
                    len = 0;
                R8_UEP0_T_LEN = len;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
            }

            R8_USB_INT_FG = RB_UIF_TRANSFER;
        }
    }
    else if(intflag & RB_UIF_BUS_RST)
    {
        R8_USB_DEV_AD = 0;
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        USBFS_DevEnumStatus = 0;
        R8_USB_INT_FG = RB_UIF_BUS_RST;
    }
    else if(intflag & RB_UIF_SUSPEND)
    {
        if(R8_USB_MIS_ST & RB_UMS_SUSPEND)
        {
            Ready = 0;
        }
        else
        {
            Ready = 1;
        }
        R8_USB_INT_FG = RB_UIF_SUSPEND;
    }
    else
    {
        R8_USB_INT_FG = intflag;
    }
}

// 发送键盘报告（8字节）
void USBFS_SendKeyboardReport(uint8_t *report)
{
    memcpy(pEP1_IN_DataBuf, report, 8);
    DevEP1_IN_Deal(8);
}

// 发送鼠标报告（4字节）
void USBFS_SendMouseReport(uint8_t *report)
{
    memcpy(pEP2_IN_DataBuf, report, 4);
    DevEP2_IN_Deal(4);
}

// 发送Raw HID数据
uint8_t USBFS_RawHID_Send(uint8_t *data, uint8_t len)
{
    if(USBFS_DevEnumStatus == 0) return 0;
    memcpy(pEP3_IN_DataBuf, data, len);
    DevEP3_IN_Deal(len);
    return 1;
}

// 全速USB设备初始化
void USBFS_Device_Init(void)
{
    pEP0_RAM_Addr = EP0_Databuf;
    pEP1_RAM_Addr = EP1_Databuf;
    pEP2_RAM_Addr = EP2_Databuf;
    pEP3_RAM_Addr = EP3_Databuf;

    USB_DeviceInit();
    USBFS_DevEnumStatus = 0;
}

// USB中断处理函数
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void USB_IRQHandler(void)
{
    USB_DevTransProcess();
}
