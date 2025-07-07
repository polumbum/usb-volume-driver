#ifndef VOLUME_MD_H
#define VOLUME_MD_H

#include <linux/workqueue.h>

struct work_data {
    struct work_struct work;
    int delta;
    bool do_work;
};

extern struct workqueue_struct *wq;
void wq_func(struct work_struct *work);

#endif // VOLUME_MD_H