import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import pickle
import json
import numpy as np

print "LOADING JSON FILE"

with open("rocPlots.json", "r") as f:
    inputs = json.load(f)

colors = ["red", "blue", "green", "orange", "black", "purple", "yellow", "pink", "maroon"]

for name, filelist in inputs.iteritems():
    plt.clf()
    
    files = filelist["files"]
    labels = filelist["labels"]

    plotRoc = plt.figure()
    plotRocAx = plotRoc.add_subplot(111)
    plotRocZ = plt.figure()
    plotRocZAx = plotRocZ.add_subplot(111)

    rocs = []
    rocsPt = []

    rocsZ = []
    rocsPtZ = []

    # colors per file
    for file1, label, color in zip(files, labels, colors):
        print "OPENING PICKLE; NAME: {0} FILE: {1}".format(name, file1)
        f1 = open(file1, "rb")
        PtCutMap = pickle.load(f1) 
        ncuts = len(PtCutMap)
        styles = list( (k, ( 2*(k+1), 2*(k+1)) ) for k in xrange(ncuts) ) 
        # styles per cut
        istyle = 0
        #for cut, style in zip(PtCutMap, styles):
        #for cut in sorted(PtCutMap.iteritems(), key=lambda (k,v): (v,k)):
        for cut in sorted(PtCutMap.iteritems()):
            style = styles[istyle]
            print "File: {0} Label: {1} Color: {2} Style: {3} Cut: {4}".format(file1, label, color, style, cut[0])
            istyle += 1

'''
        TPR = pickle.load(f1)
        FPR = pickle.load(f1)
        FPRZ = pickle.load(f1)
    
        TPRPtCut = pickle.load(f1)
        FPRPtCut = pickle.load(f1)
        FPRZPtCut = pickle.load(f1)
    
        rocs.append( plotRocAx.plot(FPR, TPR, label=label, color=color, alpha=1.0)[0])
        rocsPt.append(plotRocAx.plot(FPRPtCut, TPRPtCut, label=label+" Pt > 200 GeV", linestyle="dotted", color=color, alpha=1.0)[0])

        rocsZ.append(plotRocZAx.plot(FPRZ, TPR, label=label, color=color, alpha=1.0)[0])
        rocsPtZ.append(plotRocZAx.plot(FPRZPtCut, TPRPtCut, label=label+" Pt > 200 GeV", linestyle="dotted", color=color, alpha=1.0)[0])

    
    first_legend = plotRocAx.legend(handles=rocs, loc="lower right")
    plotRoc.gca().add_artist(first_legend)

    solidLine = mlines.Line2D([], [], color='black', marker=None, label='All events')
    dottedLine = mlines.Line2D([], [], color='black', marker=None, linestyle='dotted', label=r'$p_{T} > 200$ GeV')
    second_legend = plotRocAx.legend(handles=[solidLine, dottedLine], loc=(0.2, 0.02))

    #plotRocAx.legend(loc="upper left")
    plotRocAx.set_xlabel("FPR (ttbar)")
    plotRocAx.set_ylabel("TPR (ttbar)")
    plotRocAx.set_xlim(0.0, 0.5)
    plotRocAx.set_ylim(0.2, 1.0)
    #plotRocAx.xscale('log')
    #plotRocAx.yscale('log')
    plotRoc.savefig("roc_%s.png"%name)
    plotRoc.savefig("roc_%s.pdf"%name)
    plt.close(plotRoc)

    first_legend = plotRocZAx.legend(handles=rocsZ, loc="lower right")
    plotRocZ.gca().add_artist(first_legend)

    second_legend = plotRocZAx.legend(handles=[solidLine, dottedLine], loc=(0.2, 0.02))

    #plotRocZAx.legend(loc="lower right")
    plotRocZAx.set_xlabel("FPR (Znunu)")
    plotRocZAx.set_ylabel("TPR (ttbar)")
    plotRocZAx.set_xlim(0.0, 0.5)
    plotRocZAx.set_ylim(0.2, 1.0)
    plotRocZ.savefig("rocZ_%s.png"%name)
    plotRocZ.savefig("rocZ_%s.pdf"%name)
    plt.close(plotRocZ)
    
'''
