/* -*- Mode: C; indent-tabs-mode: nil -*- */

#include <Python.h>
#include <structmember.h>
#include <logcount.h>

typedef struct {
  PyObject_HEAD
  struct logcount lc;
} LogCount;

static int
add(LogCount *self, PyObject *args)
{
  Py_ssize_t i, n = PyTuple_Size(args);
  for (i = 0; i < n; ++i) {
    PyObject *arg = PyTuple_GetItem(args, i);
    char *buf;
    Py_ssize_t len;

    if (PyString_Check(arg)) {
      if (PyString_AsStringAndSize(arg, &buf, &len) < 0)
        return -1;

      logcount_add(&self->lc, (unsigned char *) buf, len);
    }
    else if (PyUnicode_Check(arg)) {
      buf = (char *) PyUnicode_AS_DATA(arg);
      len = PyUnicode_GET_DATA_SIZE(arg);
      logcount_add(&self->lc, (unsigned char *) buf, len);
    }
    else {
      PyObject *str = PyObject_Str(arg);
      if (!str)
        return -1;

      if (PyString_AsStringAndSize(str, &buf, &len) < 0) {
        Py_DECREF(str);
        return -1;
      }

      logcount_add(&self->lc, (unsigned char *) buf, len);
      Py_DECREF(str);
    }
  }

  return 0;
}

static void
LogCount_dealloc(LogCount *self)
{
  if (self->lc.nhash)
    logcount_finish(&self->lc);
}

static int
LogCount_init(LogCount *self, PyObject *args, PyObject *kwds)
{
  if (!PyTuple_Check(args))
    return 0;

  /* XXX handle size keyword */
  logcount_init(&self->lc, 63);
  return add(self, args);
}

static PyObject *
LogCount_gethash(LogCount *self, void *closure)
{
  return PyString_FromStringAndSize((char *) self->lc.hashes,
                                    self->lc.nhash * sizeof(int));
}

static int
LogCount_sethash(LogCount *self, PyObject *value, void *closure)
{
  if (!value) {
    PyErr_SetString(PyExc_TypeError, "cannot delete the hash attribute");
    return -1;
  }

  if (! PyString_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "hash attribute must be a string");
    return -1;
  }

  int len = PyString_Size(value);
  if (len % sizeof(int)) {
    PyErr_SetString(PyExc_ValueError, "hash attribute must be a string whose length is even mod 4");
    return -1;
  }

  int nhash = len / sizeof(int);
  if (nhash != self->lc.nhash) {
    logcount_finish(&self->lc);
    logcount_init(&self->lc, nhash);
  }

  memcpy(self->lc.hashes, PyString_AsString(value), len);
  return 0;
}

static PyGetSetDef LogCount_getsetters[] = {
  { "hashes",
    (getter) LogCount_gethash, (setter) LogCount_sethash,
    "The raw hash value used for the log count" },

  { 0 }
};

PyObject *
LogCount_add(LogCount *self, PyObject *args)
{
  if (!PyTuple_Check(args))
    return 0;

  if (add(self, args) < 0)
    return 0;

  return Py_None;
}

static PyMethodDef LogCount_methods[] = {
  { "add", (PyCFunction) LogCount_add, METH_VARARGS,
    "Adds the arguments to the log count." },
  { 0 }
};

static Py_ssize_t
LogCount_len(LogCount *self)
{
  return logcount_value(&self->lc);
}

static PySequenceMethods LogCount_sequence_methods = {
  (lenfunc) LogCount_len,   /* sq_length */
};

static PyTypeObject LogCountType = {
    PyObject_HEAD_INIT(0)
    0,                      /*ob_size*/
    "pylogcount.LogCount",  /*tp_name*/
    sizeof(LogCount),       /*tp_basicsize*/
};

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initpylogcount(void) 
{
    PyObject* m;

    LogCountType.tp_dealloc     = (destructor) LogCount_dealloc;
    LogCountType.tp_as_sequence = &LogCount_sequence_methods;
    LogCountType.tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    LogCountType.tp_methods     = LogCount_methods;
    LogCountType.tp_getset      = LogCount_getsetters;
    LogCountType.tp_init        = (initproc) LogCount_init;
    LogCountType.tp_new         = PyType_GenericNew;

    if (PyType_Ready(&LogCountType) < 0)
        return;

    m = Py_InitModule3("pylogcount", module_methods,
                       "Approximate counting.");

    if (!m)
      return;

    Py_INCREF(&LogCountType);
    PyModule_AddObject(m, "LogCount", (PyObject *) &LogCountType);
}

