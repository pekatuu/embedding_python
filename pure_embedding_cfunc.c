#define PY_SSIZE_T_CLEAN
#include <Python.h>

static int numargs=0;

/* Return the number of arguments of the application command line */
static PyObject*
emb_numargs(PyObject *self, PyObject *args)
{
    if(!PyArg_ParseTuple(args, ":numargs"))
        return NULL;
    return PyLong_FromLong(numargs);
}

// https://docs.python.org/ja/3/c-api/structures.html#c.PyMethodDef
static PyMethodDef EmbMethods[] = {
    {
        "numargs",          // ml_name  : method name
        emb_numargs,        // ml_meth  : pointer to c function
        METH_VARARGS,       // ml_flags : call flags
        "Return the number of arguments received by the process."
                            // ml_doc   : pointer to docstring
    },
    {NULL, NULL, 0, NULL}   // SENTINEL
};

// https://docs.python.org/ja/3/c-api/module.html#c.PyModuleDef
static PyModuleDef EmbModule = {
    PyModuleDef_HEAD_INIT,  // m_base: always PyModuleDef_HEAD_INIT
    "embf",     // m_name       : module_name
    NULL,       // m_doc        : pointer to docstring
    -1,         // m_size       : for sub-interperter support
    EmbMethods, // m_methodsq   : pointer to method table
    NULL,       // m_slots      :
    NULL,       // m_traverse   :
    NULL,       // m_clear      :
    NULL        // m_free       : pointer to destructor for this module
};

static PyObject*
PyInit_emb(void)
{
    // https://docs.python.org/ja/3/c-api/module.html#c.PyModule_Create
    // New reference
    return PyModule_Create(&EmbModule);
}

int
main(int argc, char *argv[])
{
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    if (argc < 3) {
        fprintf(stderr,"Usage: call pythonfile funcname [args]\n");
        return 1;
    }

    numargs = argc;
    PyImport_AppendInittab("emb", &PyInit_emb);
    Py_Initialize();
    pName = PyUnicode_DecodeFSDefault(argv[1]);
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, argv[2]);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(argc - 3);
            for (i = 0; i < argc - 3; ++i) {
                pValue = PyLong_FromLong(atoi(argv[i + 3]));
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
                    fprintf(stderr, "Cannot convert argument\n");
                    return 1;
                }
                /* pValue reference stolen here: */
                PyTuple_SetItem(pArgs, i, pValue);
            }
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
                return 1;
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", argv[2]);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", argv[1]);
        return 1;
    }
    if (Py_FinalizeEx() < 0) {
        return 120;
    }
    return 0;
}