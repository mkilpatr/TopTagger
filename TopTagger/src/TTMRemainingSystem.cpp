#include "TopTagger/TopTagger/include/TTMRemainingSystem.h"

#include "TopTagger/TopTagger/include/TopTaggerResults.h"
#include "TopTagger/CfgParser/include/Context.hh"
#include "TopTagger/CfgParser/include/CfgDocument.hh"

void TTMRemainingSystem::getParameters(const cfg::CfgDocument* cfgDoc)
{
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt("TTMRemainingSystem");

}

void TTMRemainingSystem::run(TopTaggerResults& ttResults)
{
    //List of constiturnt jets
    const std::vector<Constituent>& consituents = ttResults.getConstituents();

    //set containing all used jets
    const std::set<Constituent const *>& usedJets = ttResults.getUsedConstituents();

    TopObject& rsys = ttResults.getRsys();

    //First select the seed jet for the remaining system
    //The seed is ideally a b-jet, if no b-jet is found
    //the highest pt jet not included in a top will be chosen
    //If multiple b-jets exist, that with the highest pt
    //is chosen 

    Constituent const * seed = nullptr;

    for(auto& jet : consituents)
    {
        //check if jet is used in a top
        if(usedJets.count(&jet)) continue;

        //Take first jet not in a top (the constituents are pt ordered)
        if(seed == nullptr)
        {
            seed = &jet;
        }

        //or the first b-tagged jet if one exists
        if(jet.getBTagDisc() > CSVThresh_)
        {
            seed = &jet;
            break;
        }
    }

    //if no seed is found, abort calculation
    if(seed == nullptr) return;

    Constituent const * secondJet = nullptr;
    double mindR = 999.9;

    //Look for a companion jet to form the remaining system with the seed
    //The closest jet to the seed is chosen provided the seed + second jet mass 
    //is in the specified range 
    for(auto& jet : consituents)
    {
        //check if jet is used in a top or is the seed
        if(usedJets.count(&jet) || &jet == seed) continue;

        double dR = seed->p().DeltaR(jet.p());
        double mdijet = (seed->p() + jet.p()).M();

        //select second jet based upon dR and dijet mass
        if(dR < mindR && lowRsysMass_ < mdijet && mdijet < highRsysMass_)
        {
            mindR = dR;
            secondJet = &jet;
        }
    }

    //Form the final remaining system based upon seed and secondJet
    if(seed)      rsys.addConstituent(seed);
    if(secondJet) rsys.addConstituent(secondJet);
}
