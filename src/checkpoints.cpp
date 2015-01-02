// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    // Genesis Livenet: (  0, uint256("0x26386e3d1dfe042576122eef381d662551f33d68eb29e3a3fb5fd0cd5f25788e"))
    /*static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
	(     1, uint256("0x000003db92cab08116363d954d4bc16c1771824e0a4f381da4e4b81dca79e58c"))
	(    10, uint256("0x00000a0c52dfb43237e495a24193a57e00cbcf48e16258f0bcfaba1de9e9725b"))
	(   100, uint256("0x00000b3c485e86341405ddec93d01f00979c23d7ee1b8c559f0b7a593108c697"))
        ;*/
    // Treefunder: Why these were set to the above we will never know
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (     1, uint256("0x97ffe971bfa0d155402ab1814d825c30a5917c16b4b866db989528aa7bee5e71"))
        (    10, uint256("0xa3ef19e9aa5cb1bf3265357b2e4b1ca4bfe1cdbb13ca726e238bb68aa014edac"))
        (   100, uint256("0x267409b6d7feff42eae9f5a58e37fe2620bfffda03809713556bd7d3cc3b8eba"))
        (  1000, uint256("0xf2892fbbe6a074fd74e5c88640e56affa98114cc297ebbe24f147dedc18ebd38"))
        ( 10000, uint256("0x378c5d48f9528992f645951e37fe4c0929cd559921b5b23dfc0f844d7437cc10"))
        ( 16833, uint256("0xd58d9da7b5382348f2645834f4fa4b195e9fec56d617a992c44fb222544e4d5b"))
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1420166276, // * UNIX timestamp of last checkpoint block : From 1417994920 to 1420166276 
        100,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        7000.0     // * estimated number of transactions per day after checkpoint
    };

    // Litecoin Original (   546, uint256("0xa0fea99a6897f531600c8ae53367b126824fd6a847b2b2b73817a95b8e27e602"))
    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (   0, uint256("0xbb43407716677ded9c2c32cf82ecb3c26b2d24a552dbcb4d53257a9f4b32178a"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1365458829,
        547,
        576
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
