#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/hid.h>
#include <linux/hiddev.h>
#include <linux/input.h>
#include <linux/slab.h>

#include "volume_md.h"

#define VENDOR_ID 0x093a
#define PRODUCT_ID 0x2510
#define STEP 1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Polina Sablina");
MODULE_DESCRIPTION("Mouse driver for scroll+right button");

static struct hid_device_id mouse_table[] = {
    {HID_USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(hid, mouse_table);

struct mouse_struct {
    struct hid_device *hdev;
};

static int mouse_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    printk(KERN_INFO "MOD: mouse_probe called\n");
    struct mouse_struct *ms;

    ms = devm_kzalloc(&hdev->dev, sizeof(*ms), GFP_KERNEL);
    if (ms == NULL) {
        hid_err(hdev,"MOD: can’t alloc descriptor\n");
        return -ENOMEM;
    }

    hid_set_drvdata(hdev, ms);
    ms->hdev = hdev;

    int ret = hid_parse(hdev);
    if (ret) {
        hid_err(hdev, "MOD: parse failed\n");
        return ret;
    }

    ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (ret) {
        hid_err(hdev, "MOD: hw start failed\n");
        return ret;
    }

    return 0;
}

static void mouse_remove(struct hid_device *hdev)
{
    printk(KERN_INFO "MOD: mouse_remove called\n");
    hid_hw_stop(hdev);
}

static int mouse_raw_event(struct hid_device* hdev, struct hid_report* report, u8* data, int size)
{
    bool btn_pressed = (data[0] & 0x02) != 0;
    if (!btn_pressed)
        return 0;

    int scroll_delta = data[size-1];
    if (scroll_delta == 0x00)
        return 0;
    
    struct work_data *work_data;
    work_data = kzalloc(sizeof(*work_data), GFP_KERNEL);
    if (!work_data)
        return -ENOMEM;

    printk(KERN_INFO "MOD: Scroll: %s", scroll_delta == 0x01 ? "Up" : "Down");
    
    work_data->do_work = btn_pressed;
    if (scroll_delta == 0x01)
        work_data->delta = STEP;
    else
        work_data->delta = -STEP;
    data[size-1] = 0x00;

    INIT_WORK(&work_data->work, wq_func);
    queue_work(wq, &work_data->work);

    return 0;
}

static struct hid_driver mouse_driver = {
    .name = "usb_mouse_driver",
    .id_table = mouse_table,
    .probe = mouse_probe,
    .remove = mouse_remove,
    .raw_event = mouse_raw_event,
};

static int __init custom_mouse_init(void)
{
    printk(KERN_INFO "MOD: Loading mouse module\n");

    int ret = hid_register_driver(&mouse_driver);
    if (ret) {
        printk(KERN_ERR "MOD: Failed to register mouse driver: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "MOD: Mouse module loaded\n");
    printk(KERN_INFO "MOD: Mouse driver registered\n");

    return 0;
}

static void __exit custom_mouse_exit(void)
{
    hid_unregister_driver(&mouse_driver);
    printk(KERN_INFO "MOD: Mouse module unloaded\n");
}

module_init(custom_mouse_init);
module_exit(custom_mouse_exit);
