#include "ext-folding/utils/utils.h"

using namespace utils;

namespace c7552_3
{

static inline void copyTop(Abc_Ntk_t *pNtkTop, Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pObj, *pObjNew;
    uint i;

    // copy top
    Abc_AigConst1(pNtkTop)->pCopy = Abc_AigConst1(pNtkRes);

    Abc_NtkForEachPi(pNtkTop, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkRes);
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
        pObj->pCopy = pObjNew;
    }

    Abc_NtkForEachBox(pNtkTop, pObj, i) {
        char *liName = Abc_ObjName(Abc_ObjFanin0(pObj));
        char *loName = Abc_ObjName(Abc_ObjFanout0(pObj));
        pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), Abc_ObjName(pObj), liName, loName);
        Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);
    }

    Abc_AigForEachAnd(pNtkTop, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachBox(pNtkTop, pObj, i) {
        pObjNew = Abc_ObjFanin0(Abc_NtkBox(pNtkRes, i));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));
    }

    // collect 17 + 14 outputs
    for(i=68; i<68+17; ++i) {
        pObj = Abc_NtkPo(pNtkTop, i);
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "Top", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    for(i=97; i<Abc_NtkPoNum(pNtkTop); ++i) {
        pObj = Abc_NtkPo(pNtkTop, i);
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "Top", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    assert(Abc_NtkPoNum(pNtkRes) == 31);
}

static inline Abc_Obj_t* getCtrl(Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pLat = Abc_NtkBox(pNtkRes, 0);
    Abc_Obj_t *pLatOut = Abc_ObjFanout0(pLat);
    return (Abc_LatchIsInit0(pLat) ? Abc_ObjNot(pLatOut) : pLatOut);
}

static inline void copyM56(Abc_Ntk_t *pNtkTop, Abc_Ntk_t *pNtkM56, Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pCtrl = getCtrl(pNtkRes);
    Abc_Obj_t *pObj, *pObjNew;
    uint i;

    // copy M56: input CinX, XAbus(0~16), Not_XBbus(17~33)
    Abc_AigConst1(pNtkM56)->pCopy = Abc_AigConst1(pNtkRes);

    Abc_NtkForEachPi(pNtkM56, pObj, i) {
        if(i == 0)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 95));
        else
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i));
        pObj->pCopy = pObjNew;
    }

    assert(Abc_NtkBoxNum(pNtkM56) == 2);

    pObj = Abc_NtkBox(pNtkM56, 0);
    pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), "AdderX_0", "AdderX_0_i", "AdderX_0_o");
    Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);

    pObj = Abc_NtkBox(pNtkM56, 1);
    pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), "AdderX_1", "AdderX_1_i", "AdderX_1_o");
    Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);

    // CinX at 95th bit
    //Abc_Obj_t *pCinX = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 95));
    //pObjNew = Abc_ObjFanout0(pObjNew);
    //pObjNew = Abc_AigMux((Abc_Aig_t*)pNtkRes->pManFunc, pCtrl, pCinX, pObjNew);
    //Abc_ObjFanout0(pObj)->pCopy = pObjNew;

    Abc_AigForEachAnd(pNtkM56, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    pObj = Abc_NtkBox(pNtkM56, 0);
    pObjNew = Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-2);
    Abc_ObjAddFanin(Abc_ObjFanin0(pObjNew), Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));

    pObj = Abc_NtkBox(pNtkM56, 1);
    pObjNew = Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-1);
    Abc_ObjAddFanin(Abc_ObjFanin0(pObjNew), Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));

    // collect 17 + 2(Sum[32,33]) + 4(coutX) outputs
    Abc_NtkForEachPo(pNtkM56, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "M56", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        if((i == 15) || (i == 16)) {
            pObjNew = Abc_NtkCreatePo(pNtkRes);
            Abc_ObjAssignName(pObjNew, "M56_Dup", Abc_ObjName(pObj));
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        } else if(i == 17) {
            pObjNew = Abc_NtkCreatePo(pNtkRes);
            Abc_ObjAssignName(pObjNew, "M56_Dup1", Abc_ObjName(pObj));
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
            pObjNew = Abc_NtkCreatePo(pNtkRes);
            Abc_ObjAssignName(pObjNew, "M56_Dup2", Abc_ObjName(pObj));
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
            pObjNew = Abc_NtkCreatePo(pNtkRes);
            Abc_ObjAssignName(pObjNew, "M56_Dup3", Abc_ObjName(pObj));
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        }
    }
    assert(Abc_NtkPoNum(pNtkRes) == 31 + 23);
}

static inline void copyM8(Abc_Ntk_t *pNtkTop, Abc_Ntk_t *pNtkM8, Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pCtrl = getCtrl(pNtkRes);
    Abc_Obj_t *pObj, *pObjNew;
    uint i;

    // copy M56: input XAbus(34~50), Not_XBbus(51~67)
    Abc_AigConst1(pNtkM8)->pCopy = Abc_AigConst1(pNtkRes);

    Abc_NtkForEachPi(pNtkM8, pObj, i) {
        if(i == 0)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 96));
        else
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i+34));
        pObj->pCopy = pObjNew;
    }

    assert(Abc_NtkBoxNum(pNtkM8) == 2);

    pObj = Abc_NtkBox(pNtkM8, 0);
    pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), "AdderY_0", "AdderY_0_i", "AdderY_0_o");
    Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);

    pObj = Abc_NtkBox(pNtkM8, 1);
    pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), "AdderY_1", "AdderY_1_i", "AdderY_1_o");
    Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);

    // CinY at 96th bit
    //Abc_Obj_t *pCinY = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 96));
    //pObjNew = Abc_ObjFanout0(pObjNew);
    //pObjNew = Abc_AigMux((Abc_Aig_t*)pNtkRes->pManFunc, pCtrl, pCinY, pObjNew);
    //Abc_ObjFanout0(pObj)->pCopy = pObjNew;

    Abc_AigForEachAnd(pNtkM8, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    pObj = Abc_NtkBox(pNtkM8, 0);
    pObjNew = Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-2);
    Abc_ObjAddFanin(Abc_ObjFanin0(pObjNew), Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));

    pObj = Abc_NtkBox(pNtkM8, 1);
    pObjNew = Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-1);
    Abc_ObjAddFanin(Abc_ObjFanin0(pObjNew), Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));

    // collect 3(CoutY) + 1(CoutY_17) outputs
    Abc_NtkForEachPo(pNtkM8, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "M8", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        if(i == 1) {
            pObjNew = Abc_NtkCreatePo(pNtkRes);
            Abc_ObjAssignName(pObjNew, "M8_Dup1", Abc_ObjName(pObj));
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
            pObjNew = Abc_NtkCreatePo(pNtkRes);
            Abc_ObjAssignName(pObjNew, "M8_Dup2", Abc_ObjName(pObj));
            Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
        }
    }
    assert(Abc_NtkPoNum(pNtkRes) == 31 + 23 + 4);
}

static inline void buildSumPar(Abc_Ntk_t *pNtk)
{
    Abc_Obj_t *pCtrl = getCtrl(pNtk), *pConst0 = Abc_ObjNot(Abc_AigConst1(pNtk));
    Abc_Obj_t *pPar0, *pPar1, *pPar2, *pPar3, *pLat, *pPo;

    // Sum[0:8]
    pPar0 = pConst0;
    for(uint i=31; i<31+9; ++i)
        pPar0 = Abc_AigXor((Abc_Aig_t*)pNtk->pManFunc, pPar0, Abc_ObjChild0(Abc_NtkPo(pNtk, i)));
    pPar0 = Abc_ObjNot(pPar0);

    // Sum[9:17]
    pPar1 = pConst0;
    for(uint i=31+9; i<31+17; ++i)
        pPar1 = Abc_AigXor((Abc_Aig_t*)pNtk->pManFunc, pPar1, Abc_ObjChild0(Abc_NtkPo(pNtk, i)));
    pLat = aigUtils::aigNewLatch(pNtk, ABC_INIT_ZERO, "SumParLat", "SumParLatIn", "SumParLatOut");
    Abc_ObjAddFanin(Abc_ObjFanin0(pLat), pPar1);
    pPar1 = Abc_ObjFanout0(pLat);
    pPar1 = Abc_AigXor((Abc_Aig_t*)pNtk->pManFunc, pPar1, Abc_ObjChild0(Abc_NtkPo(pNtk, 31)));
    pPar1 = Abc_ObjNot(pPar1);

    pPo = Abc_NtkCreatePo(pNtk);
    Abc_ObjAssignName(pPo, "nsp0", NULL);
    Abc_ObjAddFanin(pPo, Abc_AigMux((Abc_Aig_t*)pNtk->pManFunc, pCtrl, pPar0, pPar1));

    // Sum[18:26]
    pPar2 = pConst0;
    for(uint i=31+1; i<31+10; ++i)
        pPar2 = Abc_AigXor((Abc_Aig_t*)pNtk->pManFunc, pPar2, Abc_ObjChild0(Abc_NtkPo(pNtk, i)));
    pPar2 = Abc_ObjNot(pPar2);
    pPo = Abc_NtkCreatePo(pNtk);
    Abc_ObjAssignName(pPo, "nsp1", NULL);
    Abc_ObjAddFanin(pPo, pPar2);

    // Sum[27:33]
    pPar3 = pConst0;
    for(uint i=31+10; i<31+17; ++i)
        pPar3 = Abc_AigXor((Abc_Aig_t*)pNtk->pManFunc, pPar3, Abc_ObjChild0(Abc_NtkPo(pNtk, i)));
    pPar3 = Abc_ObjNot(pPar3);
    pPo = Abc_NtkCreatePo(pNtk);
    Abc_ObjAssignName(pPo, "nsp2", NULL);
    Abc_ObjAddFanin(pPo, pPar3);

    assert(Abc_NtkPoNum(pNtk) == 31 + 23 + 4 + 3);
}

static inline void copyM9XA(Abc_Ntk_t *pNtkTop, Abc_Ntk_t *pNtkM9XA, Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pObj, *pObjNew;
    uint i;

    // copy M9XA: input 0~16, 85~87, 94
    Abc_AigConst1(pNtkM9XA)->pCopy = Abc_AigConst1(pNtkRes);

    Abc_NtkForEachPi(pNtkM9XA, pObj, i) {
        if(i < 17)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i));
        else if(i < 20)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i+68));
        else
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 94));
        
        pObj->pCopy = pObjNew;
    }

    Abc_NtkForEachBox(pNtkM9XA, pObj, i) {
        char liName[100], loName[100];
        sprintf(liName, "M9XA_%s", Abc_ObjName(Abc_ObjFanin0(pObj)));
        sprintf(loName, "M9XA_%s", Abc_ObjName(Abc_ObjFanout0(pObj)));
        pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), Abc_ObjName(pObj), liName, loName);
        Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);
    }

    Abc_AigForEachAnd(pNtkM9XA, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachBox(pNtkM9XA, pObj, i) {
        pObjNew = Abc_ObjFanin0(Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-3+i));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));
    }

    // collect 1 output
    Abc_NtkForEachPo(pNtkM9XA, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "M9XA", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    assert(Abc_NtkPoNum(pNtkRes) == 31 + 23 + 4 + 3 + 1);
}

static inline void copyM9YA(Abc_Ntk_t *pNtkTop, Abc_Ntk_t *pNtkM9YA, Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pObj, *pObjNew;
    uint i;

    // copy M9XA: input 34~50, 88~90, 94
    Abc_AigConst1(pNtkM9YA)->pCopy = Abc_AigConst1(pNtkRes);

    Abc_NtkForEachPi(pNtkM9YA, pObj, i) {
        if(i < 17)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i+34));
        else if(i < 20)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i+71));
        else
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 94));
        
        pObj->pCopy = pObjNew;
    }

    Abc_NtkForEachBox(pNtkM9YA, pObj, i) {
        char liName[100], loName[100];
        sprintf(liName, "M9YA_%s", Abc_ObjName(Abc_ObjFanin0(pObj)));
        sprintf(loName, "M9YA_%s", Abc_ObjName(Abc_ObjFanout0(pObj)));
        pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), Abc_ObjName(pObj), liName, loName);
        Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);
    }

    Abc_AigForEachAnd(pNtkM9YA, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachBox(pNtkM9YA, pObj, i) {
        pObjNew = Abc_ObjFanin0(Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-3+i));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));
    }

    // collect 1 output
    Abc_NtkForEachPo(pNtkM9YA, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "M9YA", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    assert(Abc_NtkPoNum(pNtkRes) == 31 + 23 + 4 + 3 + 1 + 1);
}

static inline void copyM9YB(Abc_Ntk_t *pNtkTop, Abc_Ntk_t *pNtkM9YB, Abc_Ntk_t *pNtkRes)
{
    Abc_Obj_t *pObj, *pObjNew;
    uint i;

    // copy M9XA: input 51~67, 85~87, 95
    Abc_AigConst1(pNtkM9YB)->pCopy = Abc_AigConst1(pNtkRes);

    Abc_NtkForEachPi(pNtkM9YB, pObj, i) {
        if(i < 17)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i+51));
        else if(i < 20)
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, i+74));
        else
            pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 95));
        
        pObj->pCopy = pObjNew;
    }

    Abc_NtkForEachBox(pNtkM9YB, pObj, i) {
        char liName[100], loName[100];
        sprintf(liName, "M9YB_%s", Abc_ObjName(Abc_ObjFanin0(pObj)));
        sprintf(loName, "M9YB_%s", Abc_ObjName(Abc_ObjFanout0(pObj)));
        pObjNew = aigUtils::aigNewLatch(pNtkRes, Abc_LatchInit(pObj), Abc_ObjName(pObj), liName, loName);
        Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanout0(pObjNew);
    }

    Abc_AigForEachAnd(pNtkM9YB, pObj, i)
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));

    Abc_NtkForEachBox(pNtkM9YB, pObj, i) {
        pObjNew = Abc_ObjFanin0(Abc_NtkBox(pNtkRes, Abc_NtkBoxNum(pNtkRes)-3+i));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(Abc_ObjFanin0(pObj)));
    }

    // collect 1 output
    Abc_NtkForEachPo(pNtkM9YB, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkRes);
        Abc_ObjAssignName(pObjNew, "M9YB", Abc_ObjName(pObj));
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    assert(Abc_NtkPoNum(pNtkRes) == 31 + 23 + 4 + 3 + 1 + 1 + 1);
}

static inline void buildPC0(Abc_Ntk_t *pNtkRes, Abc_Ntk_t *pNtkTop)
{
    Abc_Obj_t *pCtrl = getCtrl(pNtkRes);
    Abc_Obj_t *p97 = Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 97));
    Abc_Obj_t *p97_0 = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, pCtrl, p97);
    Abc_Obj_t *p97_1 = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, Abc_ObjNot(pCtrl), p97);

    // nand output 61~63, 
    Abc_Obj_t *pObj = Abc_AigConst1(pNtkRes);
    pObj = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, pObj, Abc_ObjChild0(Abc_NtkPo(pNtkRes, 61)));
    pObj = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, pObj, Abc_ObjChild0(Abc_NtkPo(pNtkRes, 62)));
    pObj = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, pObj, Abc_ObjChild0(Abc_NtkPo(pNtkRes, 63)));
    
    pObj = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, pObj, Abc_ObjChild0Copy(Abc_NtkPo(pNtkTop, 96)));
    pObj = Abc_AigAnd((Abc_Aig_t*)pNtkRes->pManFunc, pObj, p97_1);
    pObj = Abc_ObjNot(pObj);
    pObj = Abc_AigMux((Abc_Aig_t*)pNtkRes->pManFunc, pCtrl, p97_0, pObj);

    Abc_ObjRemoveFanins(Abc_NtkPo(pNtkRes, 17));
    Abc_ObjAddFanin(Abc_NtkPo(pNtkRes, 17), pObj);
}

int C7552_Command(Abc_Frame_t *pAbc, int argc, char **argv)
{
    char topName[] = "case7552-2/top2.blif";
    char m56Name[] = "34ad_17.n.blif";
    char m8Name[] = "34ay_17.n.blif";
    char M9xName[] = "case7552-2/M9x.n2.blif";
    char M9yName[] = "case7552-2/M9y.n2.blif";

    Abc_Ntk_t *pNtkTop = aigUtils::aigReadFromFile(topName);
    Abc_Ntk_t *pNtkM56 = aigUtils::aigReadFromFile(m56Name);
    Abc_Ntk_t *pNtkM8 = aigUtils::aigReadFromFile(m8Name);
    Abc_Ntk_t *pNtkM9XA = aigUtils::aigReadFromFile(M9xName);
    Abc_Ntk_t *pNtkM9YA = aigUtils::aigReadFromFile(M9yName);
    Abc_Ntk_t *pNtkM9YB = aigUtils::aigReadFromFile(M9xName);

    Abc_Ntk_t *pNtkRes = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    pNtkRes->pName = Extra_UtilStrsav("c7552-tm2");

    copyTop(pNtkTop, pNtkRes);
    copyM56(pNtkTop, pNtkM56, pNtkRes);
    copyM8(pNtkTop, pNtkM8, pNtkRes);
    buildSumPar(pNtkRes);
    copyM9XA(pNtkTop, pNtkM9XA, pNtkRes);
    copyM9YA(pNtkTop, pNtkM9YA, pNtkRes);
    copyM9YB(pNtkTop, pNtkM9YB, pNtkRes);
    buildPC0(pNtkRes, pNtkTop);
    
    Abc_AigCleanup((Abc_Aig_t*)pNtkRes->pManFunc);
    assert(Abc_NtkCheck(pNtkRes));
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes);

    Abc_NtkDelete(pNtkTop);
    Abc_NtkDelete(pNtkM56);
    Abc_NtkDelete(pNtkM8);
    Abc_NtkDelete(pNtkM9XA);
    Abc_NtkDelete(pNtkM9YA);
    Abc_NtkDelete(pNtkM9YB);

    return 0;
}

// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd(pAbc, "Time-frame Folding", "c7552-3", C7552_Command, 0);
}

// called during ABC termination
void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer = {init, destroy};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer);
    }
} C7552_registrar;

} // end namespace c7552_3