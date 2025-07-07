#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include <sound/core.h>
#include <sound/control.h>
#include <sound/asound.h>
#include <sound/initval.h>

#include "volume_md.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Polina Sablina");
MODULE_DESCRIPTION("Volume control module");

static struct snd_kcontrol *volume_control = NULL;

extern struct workqueue_struct *wq = NULL;
EXPORT_SYMBOL(wq);

extern void wq_func(struct work_struct *work)
{
    if (!volume_control) {
        printk(KERN_ERR "MOD: Volume control not initialized\n");
        kfree((void*)work);
        return;
    }

    struct work_data *data = (struct work_data *)work;
    int delta = data->delta;
    bool do_work = data->do_work;
    if(!do_work) {
        kfree((void*)work);
        return;
    }

    struct snd_ctl_elem_value *elem_value;
    struct snd_ctl_elem_info *elem_info;
    long min_val, max_val;
    
    elem_value = kzalloc(sizeof(*elem_value), GFP_KERNEL);
    elem_info = kzalloc(sizeof(*elem_info), GFP_KERNEL);
    if(!elem_value || !elem_info) {
        printk(KERN_ERR "MOD: Failed to allocate memory for elem_value or elem_info\n");
        if(elem_value) kfree(elem_value);
        if(elem_info) kfree(elem_info);
        kfree((void*)work);
        return;
    }

    elem_info->id = volume_control->id;
    int err = volume_control->info(volume_control, elem_info);
    if(err < 0){
        printk(KERN_ERR "MOD: Failed to get info about the volume control\n");
        kfree(elem_info);
        kfree(elem_value);
        kfree((void*)work);
        return;
    }
    
    if (elem_info->type != SNDRV_CTL_ELEM_TYPE_INTEGER) {
        printk(KERN_ERR "MOD: Control is not of INTEGER type\n");
        kfree(elem_info);
        kfree(elem_value);
        kfree((void*)work);
        return;
    }
    
    min_val = elem_info->value.integer.min;
    max_val = elem_info->value.integer.max;

    err = volume_control->get(volume_control, elem_value);
    if(err < 0) {
        printk(KERN_ERR "MOD: Failed to get volume value\n");
        kfree(elem_value);
        kfree(elem_info);
            kfree((void*)work);;
        return;
    }
    
    for (int i = 0; i < elem_info->count; i++) {
        int cur_volume = elem_value->value.integer.value[i];
        int new_raw_volume = cur_volume + delta;
        if (new_raw_volume < min_val)
            new_raw_volume = min_val;
        if (new_raw_volume > max_val)
            new_raw_volume = max_val;
        elem_value->value.integer.value[i] = new_raw_volume;
        printk(KERN_INFO "MOD: Min = %ld, Max = %ld, Current Volume = %d, New Raw Volume = %d\n",
            min_val, max_val, cur_volume, new_raw_volume);
    }

    err = volume_control->put(volume_control, elem_value);
    if (err < 0)
        printk(KERN_ERR "MOD: Failed to set volume value\n");

    kfree(elem_value);
    kfree(elem_info);
    kfree((void*)work);
}
EXPORT_SYMBOL(wq_func);

static int init_volume_ctl(void)
{
    struct snd_card *card = NULL;
    int card_index = 0;

    wq = create_singlethread_workqueue("wq");
    if (!wq) {
        printk(KERN_ERR "MOD: Failed to init workqueue\n");
        return -ENOMEM;
    }

    while(1) {
        card = snd_card_ref(card_index);
        if (!card)
            break;

        struct snd_ctl_elem_id *id;
        id = kzalloc(sizeof(*id), GFP_KERNEL);
        if (!id) {
            printk(KERN_ERR "MOD: Failed to allocate memory for id");
            return -ENOMEM;
        }

        strcpy(id->name, "Master Playback Volume");
        id->iface = SNDRV_CTL_ELEM_IFACE_MIXER;

        volume_control = snd_ctl_find_id(card, id);
        kfree(id);
        if (!volume_control) {
            printk(KERN_ERR "MOD: Volume control not found on card %d, trying next\n", card_index);
            snd_card_unref(card);
            card_index++;
            continue;
        } else {
            printk(KERN_INFO "MOD: Volume control initialized on card %d\n", card_index);
            break;
        }
    }

    if(!card){
        printk(KERN_ERR "MOD: No card found with suitable volume control\n");
        if (wq) {
            destroy_workqueue(wq);
            wq = NULL;
        }
        return -ENODEV;
    }

    return 0;
}

static int __init volume_init(void)
{
    printk(KERN_INFO "MOD: Loading volume module\n");

    int ret = init_volume_ctl();
    if(ret < 0) {
        printk(KERN_ERR "MOD: Failed to init volume control: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "MOD: Volume module loaded\n");
    return 0;
}

static void __exit volume_exit(void)
{
    printk(KERN_INFO "MOD: Volume module unloaded\n");
}

module_init(volume_init);
module_exit(volume_exit);
