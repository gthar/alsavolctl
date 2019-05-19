#include "Python.h"
#include <stdbool.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <alsa/version.h>

typedef struct {
    PyObject_HEAD;

    char *card;
    char *device;
    char *mixer;

    long volume_state;
    int switch_state;
    long pmin;
    long pmax;

    snd_ctl_t *ctl;

} Mixer;

int open_ctl(const char *card_name, snd_ctl_t **ctl)
{
    int err;
    err = snd_ctl_open(ctl, card_name, SND_CTL_READONLY);
    if (err < 0) return err;
    err = snd_ctl_subscribe_events(*ctl, 1);
    if (err < 0) return err;
    return 0;
}

snd_mixer_elem_t * get_elem(snd_mixer_t *handle, const char *mixer_name)
{
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, mixer_name);
    return snd_mixer_find_selem(handle, sid);
}

int init_handle(const char *device, snd_mixer_t **handle)
{
    int err;
    err = snd_mixer_open(handle, 0);
    if (err < 0) return err;
    err = snd_mixer_attach(*handle, device);
    if (err < 0) return err;
    err = snd_mixer_selem_register(*handle, NULL, NULL);
    if (err < 0) return err;
    err = snd_mixer_load(*handle);
    if (err < 0) return err;
    return 0;
}

static void Mixer_dealloc(Mixer *self)
{
    snd_ctl_close(self->ctl);
    free(self->card);
    free(self->mixer);
    free(self->device);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Mixer_getvolume(Mixer *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromLong(self->volume_state);
}

static PyObject *
Mixer_getswitch(Mixer *self, PyObject *Py_UNUSED(ignored))
{
    return PyBool_FromLong(self->switch_state);
}

static PyObject *
Mixer_getrange(Mixer *self, PyObject *Py_UNUSED(ignored))
{
    return PyTuple_Pack(
               2,
               PyLong_FromLong(self->pmin),
               PyLong_FromLong(self->pmax));
}

static PyObject *
Mixer_setvolume(Mixer *self, PyObject *args)
{
    int i;
    long val;

    snd_mixer_elem_t *elem;
    snd_mixer_t *handle;

    if (!PyArg_ParseTuple(args, "l", &val))
        return NULL;

    init_handle(self->device, &handle);
    elem = get_elem(handle, self->mixer);

    for (i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
        if (snd_mixer_selem_has_playback_channel(elem, i)) {
            snd_mixer_selem_set_playback_volume(elem, i, val);
        }
    }

    snd_mixer_close(handle);

    return Py_None;
}

static PyObject *
Mixer_setswitch(Mixer *self, PyObject *args)
{
    int i;
    int val;

    snd_mixer_elem_t *elem;
    snd_mixer_t *handle;

    if (!PyArg_ParseTuple(args, "i", &val))
        return NULL;

    init_handle(self->device, &handle);
    elem = get_elem(handle, self->mixer);

    for (i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
        if (snd_mixer_selem_has_playback_channel(elem, i)) {
            snd_mixer_selem_set_playback_switch(elem, i, val);
        }
    }

    snd_mixer_close(handle);

    return Py_None;
}

static PyObject *
Mixer_update(Mixer *self, PyObject *Py_UNUSED(ignored))
{
    snd_mixer_t *handle = 0;
    snd_mixer_elem_t *elem;

    init_handle(self->device, &handle);
    elem = get_elem(handle, self->mixer);

    snd_mixer_selem_get_playback_volume(elem, 0, &self->volume_state);
    snd_mixer_selem_get_playback_switch(elem, 0, &self->switch_state);
    snd_mixer_selem_get_playback_volume_range(elem, &self->pmin, &self->pmax);

    snd_mixer_close(handle);

    return Py_None;
}

bool check_event (snd_ctl_t *ctl)
{
    snd_ctl_event_t *event;
    struct pollfd fd;
    unsigned int mask;
    unsigned short revents;

    snd_ctl_poll_descriptors(ctl, &fd, 1);

    Py_BEGIN_ALLOW_THREADS
    poll(&fd, 1, -1);  // we wait here for an event
    Py_END_ALLOW_THREADS

    snd_ctl_poll_descriptors_revents(ctl, &fd, 1, &revents);
    if (revents & POLLIN) {

        snd_ctl_event_alloca(&event);
        snd_ctl_read(ctl, event);

        if (snd_ctl_event_get_type(event) != SND_CTL_EVENT_ELEM)
            return false;

        mask = snd_ctl_event_elem_get_mask(event);
        if (!(mask & SND_CTL_EVENT_MASK_VALUE))
            return false;

        return true;

    }
    return false;
}

static PyObject *
Mixer_wait_for_event(Mixer *self, PyObject *Py_UNUSED(ignored))
{
    long new_volume;
    int new_switch;
    int vol_change = false;
    int switch_change = false;

    snd_mixer_t *handle = 0;
    snd_mixer_elem_t *elem;

    PyObject* res = PyDict_New();

    while (!(vol_change || switch_change)) {
        if (check_event(self->ctl)) {
            init_handle(self->device, &handle);
            elem = get_elem(handle, self->mixer);

            snd_mixer_selem_get_playback_volume(elem, 0, &new_volume);
            snd_mixer_selem_get_playback_switch(elem, 0, &new_switch);

            snd_mixer_close(handle);

            vol_change = new_volume != self->volume_state;
            switch_change = new_switch != self->switch_state;

            if (vol_change) {
                self->volume_state = new_volume;
                PyDict_SetItemString(
                    res,
                    "volume",
                    PyLong_FromLong(new_volume));
            }

            if (switch_change) {
                self->switch_state = new_switch;
                PyDict_SetItemString(
                    res,
                    "switch",
                    PyBool_FromLong(new_switch));
            }
        }
    }

    return res;
}

static PyMethodDef Mixer_methods[] = {
    {"getvolume",
        (PyCFunction) Mixer_getvolume, METH_VARARGS,
        "get the volume value"},
    {"setvolume",
        (PyCFunction) Mixer_setvolume, METH_VARARGS,
        "set the volume value"},
    {"getswitch",
        (PyCFunction) Mixer_getswitch, METH_VARARGS,
        "get the switch value"},
    {"setswitch",
        (PyCFunction) Mixer_setswitch, METH_VARARGS,
        "set the switch value"},
    {"getrange",
        (PyCFunction) Mixer_getrange, METH_VARARGS,
        "get the volume range"},
    {"update",
        (PyCFunction) Mixer_update, METH_VARARGS,
        "uptade the volume, switch and range values"},
    {"wait_for_event",
        (PyCFunction) Mixer_wait_for_event, METH_VARARGS,
        "wait for a change in volume or switch_state"},
    {NULL}
};

static PyObject *
Mixer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Mixer *self;
    self = (Mixer *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->card = NULL;
        self->device = NULL;
        self->mixer = NULL;
        self->ctl = NULL;
        self->volume_state = 0;
        self-> switch_state = 0;
        self->pmin = 0;
        self->pmax = 0;
    }

    return (PyObject *) self;
}

static int
Mixer_init(Mixer *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = { "card", "device", "mixer", NULL };

    char *card, *device, *mixer;
    int err;
    snd_mixer_t *handle = 0;
    snd_mixer_elem_t *elem;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sss", kwlist,
                                     &card, &device, &mixer)) {
        PyErr_SetString(PyExc_SystemExit, "not able to parse arguments\n");
        PyErr_Print();
        return -1;
    }

    self->card = strdup(card);
    self->device = strdup(device);
    self->mixer = strdup(mixer);

    err = init_handle(device, &handle);
    if (err < 0) {
        PyErr_Format(
            PyExc_SystemExit,
            "Error while getting the handle for device %s\n",
            device);
        PyErr_Print();
        return -1;
    }
    elem = get_elem(handle, mixer);
    if (!elem) {
        snd_mixer_close(handle);
        PyErr_Format(PyExc_SystemExit, "Cannot find mixer %s\n", mixer);
        PyErr_Print();
        return -1;
    }

    snd_mixer_selem_get_playback_volume(elem, 0, &self->volume_state);
    snd_mixer_selem_get_playback_switch(elem, 0, &self->switch_state);
    snd_mixer_selem_get_playback_volume_range(elem, &self->pmin, &self->pmax);

    snd_mixer_close(handle);

    err = open_ctl(card, &self->ctl);
    if (err < 0) {
        PyErr_Format(PyExc_SystemExit, "Cannot open card %s\n", card);
        PyErr_Print();
        return -1;
    }

    return 0;
}

static PyTypeObject MixerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_mixer.Mixer",
    .tp_doc = "my mixer object",
    .tp_basicsize = sizeof(Mixer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Mixer_new,
    .tp_init = (initproc) Mixer_init,
    .tp_dealloc = (destructor) Mixer_dealloc,
    .tp_methods = Mixer_methods
};

static struct PyModuleDef _mixer_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_mixer",
    .m_doc = "My C module to monitor an control ALSA volume",
    .m_size = -1,
};

PyObject *PyInit__mixer(void)
{
    PyObject *m;
    if (PyType_Ready(&MixerType) < 0)
        return NULL;

    m = PyModule_Create(&_mixer_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&MixerType);
    PyModule_AddObject(m, "Mixer", (PyObject *) &MixerType);
    return m;
}
