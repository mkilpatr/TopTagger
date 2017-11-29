#include "TopTagger/TopTagger/include/TTMTFPyBind.h"

#include "TopTagger/TopTagger/include/TopTaggerUtilities.h"
#include "TopTagger/TopTagger/include/TopObject.h"
#include "TopTagger/TopTagger/include/TopTaggerResults.h"
#include "TopTagger/CfgParser/include/Context.hh"
#include "TopTagger/CfgParser/include/CfgDocument.hh"
#include "TopTagger/CfgParser/include/TTException.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <iostream>

#ifdef DOTENSORFLOWPYBIND
static std::string embeddedTensorflowScript = ""
"import os\n"
"import numpy as np\n"
"import tensorflow as tf\n"
"sess = x = y = None\n"
"\n"
"def load_graph(frozen_graph_filename):\n"
"    graph_def = tf.GraphDef()\n"
"    with tf.gfile.GFile(frozen_graph_filename, \"rb\") as f:\n"
"        graph_def.ParseFromString(f.read())\n"
"    with tf.Graph().as_default() as graph:\n"
"        tf.import_graph_def(graph_def, input_map=None, return_elements=None, name=\"\", op_dict=None, producer_op_list=None)\n"
"    return graph\n"
"\n"
"def start_session(frozen_graph_filename):\n"
"    global sess\n"
"    graph = load_graph(frozen_graph_filename)\n"
"    sess = tf.Session(graph=graph)\n"
"\n"
"def eval_session(inputs, outputs):\n"
"    outputs.update(sess.run(dict(zip(outputs.keys(), outputs.keys())), feed_dict=inputs))\n";
#endif

void TTMTFPyBind::getParameters(const cfg::CfgDocument* cfgDoc, const std::string& localContextName)
{
#ifdef DOTENSORFLOWPYBIND
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt(localContextName);

    discriminator_ = cfgDoc->get("discCut",       localCxt, -999.9);
    discOffset_    = cfgDoc->get("discOffset",    localCxt, 999.9);
    discSlope_     = cfgDoc->get("discSlope",     localCxt, 0.0);
    modelFile_     = cfgDoc->get("modelFile",     localCxt, "");
    NConstituents_ = cfgDoc->get("NConstituents", localCxt, 3);
    inputOp_       = cfgDoc->get("inputOp",       localCxt, "x");
    outputOp_      = cfgDoc->get("outputOp",      localCxt, "y");

    csvThreshold_  = cfgDoc->get("csvThreshold", localCxt, -999.9);
    bEtaCut_       = cfgDoc->get("bEtaCut",      localCxt, -999.9);
    maxNbInTop_    = cfgDoc->get("maxNbInTop",   localCxt, -1);

    int iVar = 0;
    bool keepLooping;
    do
    {
        keepLooping = false;

        //Get variable name
        std::string varName = cfgDoc->get("mvaVar", iVar, localCxt, "");

        //if it is a non empty string save in vector
        if(varName.size() > 0)
        {
            keepLooping = true;

            vars_.push_back(varName);
        }
        ++iVar;
    }
    while(keepLooping);

    initializePyInterpreter();

    //create function arguements tuple
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, PyString_FromString(modelFile_.c_str()));

    //start the tensorflow session
    callPython("start_session", pArgs);

    Py_DECREF(pArgs);

    //create numpy array for input data
    npy_intp sizearray[2] = {1, static_cast<npy_intp>(vars_.size())};
    //PyObject* nparray_ = PyArray_SimpleNewFromData(2, sizearray, NPY_FLOAT, data);
    nparray_ = PyArray_SimpleNew(2, sizearray, NPY_FLOAT);

    // create input feed dict
    inputs_ = PyDict_New();
    PyDict_SetItemString(inputs_, inputOp_.c_str(), nparray_);

    //load variables
    if(NConstituents_ == 1)
    {
        varCalculator_.reset(new ttUtility::BDTMonojetInputCalculator());
    }
    else if(NConstituents_ == 2)
    {
        varCalculator_.reset(new ttUtility::BDTDijetInputCalculator());
    }
    else if(NConstituents_ == 3)
    {
        varCalculator_.reset(new ttUtility::TrijetInputCalculator());
    }
    //map variables
    varCalculator_->mapVars(vars_, static_cast<float*>(PyArray_GETPTR2(reinterpret_cast<PyArrayObject*>(nparray_), 0, 0)));


#else
    THROW_TTEXCEPTION("ERROR: TopTagger not compiled with python support!!!");
#endif
}

void TTMTFPyBind::run(TopTaggerResults& ttResults)
{
#ifdef DOTENSORFLOWPYBIND
    //Get the list of top candidates as generated by the clustering algo
    std::vector<TopObject>& topCandidates = ttResults.getTopCandidates();
    //Get the list of final tops into which we will stick candidates
    std::vector<TopObject*>& tops = ttResults.getTops();

    // create dict of output nodes
    PyObject *outputs = PyDict_New();
    PyObject* outputOpName = PyString_FromString(outputOp_.c_str());
    PyDict_SetItem(outputs, outputOpName, Py_None);

    // create arguements tuple
    PyObject *pArgs = PyTuple_New(2);
    PyTuple_SetItem(pArgs, 0, inputs_);
    PyTuple_SetItem(pArgs, 1, outputs);
    //Tuple will steal reference to inputs_, but we want it around after the tuple is destroyed
    Py_INCREF(inputs_);

    for(auto& topCand : topCandidates)
    {
        //We only want to apply the MVA algorithm to triplet tops
        if(topCand.getNConstituents() == 3)
        {
            //Prepare data from top candidate
            varCalculator_->calculateVars(topCand);

            //Run python session to network on input data
            callPython("eval_session", pArgs);

            //Get output discriminator
            PyObject *discriminators = PyDict_GetItem(outputs, outputOpName);
            double discriminator = -999.9;
            if(PyArray_Check(discriminators))
            {
                discriminator = static_cast<double>(*static_cast<float*>(PyArray_GETPTR2(reinterpret_cast<PyArrayObject*>(discriminators), 0, 0)));
                topCand.setDiscriminator(discriminator);
            }
            else
            {
                THROW_TTEXCEPTION("Returned object is not a numpy array!!!");
            }

            //Check number of b-tagged jets in the top
            bool passBrequirements = maxNbInTop_ < 0 || topCand.getNBConstituents(csvThreshold_, bEtaCut_) <= maxNbInTop_;

            //place in final top list if it passes the threshold
            if(discriminator > std::min(discriminator_, discOffset_ + topCand.p().Pt()*discSlope_) && passBrequirements)
            {
                tops.push_back(&topCand);
            }
        }
    }

    Py_DECREF(outputs);
    Py_DECREF(outputOpName);
    Py_DECREF(pArgs);

#endif
}

#ifdef DOTENSORFLOWPYBIND

TTMTFPyBind::~TTMTFPyBind()
{
    //finish cleanup of python objects and close interpreter
    Py_DECREF(inputs_);
    Py_DECREF(nparray_);
    Py_DECREF(pModule_);
    Py_DECREF(pMain_);
    Py_DECREF(pGlobal_);
    Py_Finalize();
}


void TTMTFPyBind::initializePyInterpreter()
{
    // initialize the python interpreter
    if(!Py_IsInitialized())
    {
        PyEval_InitThreads();
        Py_Initialize();
    }

    // create the main module
    pMain_ = PyImport_AddModule("__main__");

    // define the globals of the main module as our context
    pGlobal_ = PyModule_GetDict(pMain_);

    // since PyModule_GetDict returns a borrowed reference, increase the count to own one
    Py_INCREF(pGlobal_);

    // load the python interface module
    pModule_ = PyRun_String(embeddedTensorflowScript.c_str(), Py_file_input, pGlobal_, pGlobal_);

    // initialize numpy api
    import_array();
}

PyObject* TTMTFPyBind::callPython(const std::string& func, PyObject* pArgs)
{
    if (pModule_ != NULL && pMain_ != NULL)
    {
        PyObject* pFunc = PyObject_GetAttrString(pMain_, func.c_str());
        // pFunc is a new reference

        if (pFunc && PyCallable_Check(pFunc))
        {
            PyObject* pValue = PyObject_CallObject(pFunc, pArgs);

            if (pValue != NULL)
            {
                Py_DECREF(pValue);
            }
            else
            {
                Py_DECREF(pFunc);
                Py_DECREF(pModule_);
                Py_DECREF(pMain_);
                PyErr_Print();
                THROW_TTEXCEPTION("Cannot call function \"" + func + "\"!!!");
            }
        }
        else
        {
            if (PyErr_Occurred())
                PyErr_Print();
            THROW_TTEXCEPTION("Cannot find function \"" + func + "\"!!!");
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule_);
    }
    else
    {
        PyErr_Print();
        THROW_TTEXCEPTION("Python interpreter not initialized!!!");
    }

    return NULL;
}

#endif