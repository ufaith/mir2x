/*
 * =====================================================================================
 *
 *       Filename: editormap.cpp
 *        Created: 02/08/2016 22:17:08
 *  Last Modified: 03/24/2017 16:56:13
 *
 *    Description: EditorMap has no idea of ImageDB, WilImagePackage, etc..
 *                 Use function handler to handle draw, cache, etc
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */
#include "mir2map.hpp"
#include "mir2xmap.hpp"
#include "editormap.hpp"
#include "supwarning.hpp"

#include <memory.h>
#include "assert.h"
#include <cstring>
#include <functional>
#include <cstdint>
#include <algorithm>
#include <vector>
#include "savepng.hpp"
#include "filesys.hpp"
#include "sysconst.hpp"
#include "mathfunc.hpp"

#include <FL/fl_ask.H>

EditorMap::EditorMap()
    : m_W(0)
    , m_H(0)
    , m_Valid(false)
    , m_Mir2Map(nullptr)
    , m_Mir2xMap(nullptr)
    , m_Mir2xMapData(nullptr)
{
    std::memset(m_bAniTileFrame, 0, sizeof(uint8_t) * 8 * 16);
    std::memset(m_dwAniSaveTime, 0, sizeof(uint32_t) * 8);
}

EditorMap::~EditorMap()
{
    delete m_Mir2xMapData; m_Mir2xMapData = nullptr;
    delete m_Mir2xMap    ; m_Mir2xMap     = nullptr;
    delete m_Mir2Map     ; m_Mir2Map      = nullptr;
}

void EditorMap::ExtractOneTile(int nXCnt, int nYCnt, std::function<void(uint8_t, uint16_t)> fnWritePNG)
{
    if(!Valid() || !ValidC(nXCnt, nYCnt)
            || (nXCnt % 2) || (nYCnt % 2) || !TileValid(nXCnt, nYCnt)){ return; }

    uint32_t nDescKey    = Tile(nXCnt, nYCnt);
    uint8_t  nFileIndex  = (uint8_t)((nDescKey & 0X00FF0000) >> 16);
    uint16_t nImageIndex = (uint16_t)((nDescKey & 0X0000FFFF));

    fnWritePNG(nFileIndex, nImageIndex);
}

void EditorMap::ExtractTile(std::function<void(uint8_t, uint16_t)> fnWritePNG)
{
    if(!Valid()){ return; }

    for(int nXCnt = 0; nXCnt < W(); nXCnt++){
        for(int nYCnt = 0; nYCnt < H(); ++nYCnt){
            if(!(nXCnt % 2) && !(nYCnt % 2)){
                ExtractOneTile(nXCnt, nYCnt, fnWritePNG);
            }
        }
    }
}

void EditorMap::DrawTile(int nCX, int nCY, int nCW,  int nCH,
        std::function<void(uint8_t, uint16_t, int, int)> fnDrawTile)
{
    if(!Valid()){ return; }

    for(int nY = nCY; nY < nCY + nCH; ++nY){
        for(int nX = nCX; nX < nCX + nCW; ++nX){
            if(!ValidC(nX, nY)){
                continue;
            }

            if(nX % 2 || nY % 2){
                continue;
            }
            
            if(!TileValid(nX, nY)){
                continue;
            }

            uint32_t nDescKey    = Tile(nX, nY);
            uint8_t  nFileIndex  = (uint8_t)((nDescKey & 0X00FF0000) >> 16);
            uint16_t nImageIndex = (uint16_t)((nDescKey & 0X0000FFFF));

            // provide cell-coordinates on map
            // fnDrawTile should convert it to drawarea pixel-coordinates
            fnDrawTile(nFileIndex, nImageIndex, nX, nY);
        }
    }
}

void EditorMap::ExtractOneObject(int nXCnt, int nYCnt, int nIndex, std::function<void(uint8_t, uint16_t, uint32_t)> fnWritePNG)
{
    if(!Valid() || !ValidC(nXCnt, nYCnt) || !ObjectValid(nXCnt, nYCnt, nIndex)){ return; }

    uint32_t nKey = Object(nXCnt, nYCnt, nIndex);

    bool     bBlend      = (AlphaObjectValid(nXCnt, nYCnt, nIndex) != 0);
    uint8_t  nFileIndex  = ((nKey & 0X00FF0000) >> 16);
    uint16_t nImageIndex = ((nKey & 0X0000FFFF));
    int      nAniCnt     = ((nKey & 0X0F000000) >> 24);

    int      nFrameCount = (AniObjectValid(nXCnt, nYCnt, nIndex) ? nAniCnt : 1);
    uint32_t nImageColor = (bBlend ? 0X80FFFFFF : 0XFFFFFFFF);

    for(int nIndex = 0; nIndex < nFrameCount; ++nIndex){
        fnWritePNG(nFileIndex, nImageIndex + (uint16_t)nIndex, nImageColor);
    }
}

void EditorMap::ExtractObject(std::function<void(uint8_t, uint16_t, uint32_t)> fnWritePNG)
{
    if(!Valid()){ return; }

    for(int nYCnt = 0; nYCnt < H(); ++nYCnt){
        for(int nXCnt = 0; nXCnt < W(); ++nXCnt){
            ExtractOneObject(nXCnt, nYCnt, 0, fnWritePNG);
            ExtractOneObject(nXCnt, nYCnt, 1, fnWritePNG);
        }
    }
}

void EditorMap::DrawObject(int nCX, int nCY, int nCW, int nCH, bool bGround,
        std::function<void(uint8_t, uint16_t, int, int)> fnDrawObj, std::function<void(int, int)> fnDrawExt)
{
    if(!Valid()){ return; }
    for(int nYCnt = nCY; nYCnt < nCY + nCH; ++nYCnt){
        for(int nXCnt = nCX; nXCnt < nCX + nCW; ++nXCnt){
            // // we should draw actors, extensions here, but I delay it after the over-ground
            // // object drawing, it's really wired but works
            // if(!bGround){ fnDrawExt(nXCnt, nYCnt); }

            // 2. regular draw
            for(int nIndex = 0; nIndex < 2; ++nIndex){
                if(ValidC(nXCnt, nYCnt)
                        && ObjectValid(nXCnt, nYCnt, nIndex)
                        && bGround == GroundObjectValid(nXCnt, nYCnt, nIndex)){

                    uint32_t nKey         = Object(nXCnt, nYCnt, nIndex);
                    uint8_t  nFileIndex   = ((nKey & 0X00FF0000) >> 16);
                    uint16_t nImageIndex  = ((nKey & 0X0000FFFF));
                    int      nAniType     = ((nKey & 0X70000000) >> 28);
                    int      nAniCnt      = ((nKey & 0X0F000000) >> 24);

                    if(AniObjectValid(nXCnt, nYCnt, nIndex)){
                        nImageIndex += ObjectOff(nAniType, nAniCnt);
                    }

                    fnDrawObj(nFileIndex, nImageIndex, nXCnt, nYCnt);
                }
            }

            // put the actors, extensions rendering here, it's really really wired but
            // works, tricky part for mir2 resource maker
            // if(!bGround){ fnDrawExt(nXCnt, nYCnt); }
        }

        for(int nXCnt = nCX; nXCnt < nCX + nCW; ++nXCnt){
            if(!bGround){ fnDrawExt(nXCnt, nYCnt); }
        }
    }
}

void EditorMap::UpdateFrame(int nLoopTime)
{
    // m_bAniTileFrame[i][j]:
    //     i: denotes how fast the animation is.
    //     j: denotes how many frames the animation has.

    if(!Valid()){ return; }

    uint32_t dwDelayMS[] = {150, 200, 250, 300, 350, 400, 420, 450};

    for(int nCnt = 0; nCnt < 8; ++nCnt){
        m_dwAniSaveTime[nCnt] += nLoopTime;
        if(m_dwAniSaveTime[nCnt] > dwDelayMS[nCnt]){
            for(int nFrame = 0; nFrame < 16; ++nFrame){
                m_bAniTileFrame[nCnt][nFrame]++;
                if(m_bAniTileFrame[nCnt][nFrame] >= nFrame){
                    m_bAniTileFrame[nCnt][nFrame] = 0;
                }
            }
            m_dwAniSaveTime[nCnt] = 0;
        }
    }
}

bool EditorMap::Resize(
        int nX, int nY, int nW, int nH, // define a region on original map
        int nNewX, int nNewY,           // where the cropped region start on new map
        int nNewW, int nNewH)           // size of new map
{
    // we only support 2M * 2N cropping and expansion
    if(!Valid()){ return false; }

    nX = (std::max)(0, nX);
    nY = (std::max)(0, nY);

    if(nX % 2){ nX--; nW++; }
    if(nY % 2){ nY--; nH++; }
    if(nW % 2){ nW++; }
    if(nH % 2){ nH++; }

    nW = (std::min)(nW, W() - nX);
    nH = (std::min)(nH, H() - nY);

    if(true
            && nX == 0
            && nY == 0
            && nW == W()
            && nH == H()
            && nNewX == 0
            && nNewY == 0
            && nNewW == nW
            && nNewH == nH)
    {
        // no need to do anything
        return true;
    }

    // here (nX, nY, nW, nH) has been a valid subsection of original map
    // could be null
    //
    // invalid new map defination
    if(nNewW <= 0 || nNewH <= 0){
        return false;
    }

    // start for new memory allocation
    auto stOldBufLightMark        = m_BufLightMark;
    auto stOldBufLight            = m_BufLight;

    auto stOldBufTileMark         = m_BufTileMark;
    auto stOldBufTile             = m_BufTile;

    auto stOldBufObjMark          = m_BufObjMark;
    auto stOldBufGroundObjMark    = m_BufGroundObjMark;
    auto stOldBufAlphaObjMark     = m_BufAlphaObjMark;
    auto stOldBufAniObjMark       = m_BufAniObjMark;
    auto stOldBufObj              = m_BufObj;

    auto stOldBufGround           = m_BufGround;
    auto stOldBufGroundMark       = m_BufGroundMark;
    auto stOldBufGroundSelectMark = m_BufGroundSelectMark;

    // this function will clear the new buffer
    // with all zeros
    MakeBuf(nNewW, nNewH);

    for(int nTY = 0; nTY < nH; ++nTY){
        for(int nTX = 0; nTX < nW; ++nTX){
            int nSrcX = nTX + nX;
            int nSrcY = nTY + nY;
            int nDstX = nTX + nNewX;
            int nDstY = nTY + nNewY;

            if(nDstX >= 0 && nDstX < nNewW && nDstY >= 0 && nDstY < nNewH){
                if(!(nDstX % 2) && !(nDstY % 2) && !(nSrcX % 2) && !(nSrcY % 2)){
                    m_BufTile    [nDstX / 2][nDstY / 2] = stOldBufTile    [nSrcX / 2][nSrcY / 2];
                    m_BufTileMark[nDstX / 2][nDstY / 2] = stOldBufTileMark[nSrcX / 2][nSrcY / 2];
                }

                m_BufLight            [nDstX / 2][nDstY / 2]    = stOldBufLight            [nSrcX / 2][nSrcY / 2]   ;
                m_BufLightMark        [nDstX / 2][nDstY / 2]    = stOldBufLightMark        [nSrcX / 2][nSrcY / 2]   ;

                m_BufObj              [nDstX / 2][nDstY / 2][0] = stOldBufObj              [nSrcX / 2][nSrcY / 2][0];
                m_BufObj              [nDstX / 2][nDstY / 2][1] = stOldBufObj              [nSrcX / 2][nSrcY / 2][1];

                m_BufObjMark          [nDstX / 2][nDstY / 2][0] = stOldBufObjMark          [nSrcX / 2][nSrcY / 2][0];
                m_BufObjMark          [nDstX / 2][nDstY / 2][1] = stOldBufObjMark          [nSrcX / 2][nSrcY / 2][1];

                m_BufGroundObjMark    [nDstX / 2][nDstY / 2][0] = stOldBufGroundObjMark    [nSrcX / 2][nSrcY / 2][0];
                m_BufGroundObjMark    [nDstX / 2][nDstY / 2][1] = stOldBufGroundObjMark    [nSrcX / 2][nSrcY / 2][1];

                m_BufAlphaObjMark     [nDstX / 2][nDstY / 2][0] = stOldBufAlphaObjMark     [nSrcX / 2][nSrcY / 2][0];
                m_BufAlphaObjMark     [nDstX / 2][nDstY / 2][1] = stOldBufAlphaObjMark     [nSrcX / 2][nSrcY / 2][1];

                m_BufAniObjMark       [nDstX / 2][nDstY / 2][0] = stOldBufAniObjMark       [nSrcX / 2][nSrcY / 2][0];
                m_BufAniObjMark       [nDstX / 2][nDstY / 2][1] = stOldBufAniObjMark       [nSrcX / 2][nSrcY / 2][1];

                m_BufGround           [nDstX / 2][nDstY / 2][0] = stOldBufGround           [nSrcX / 2][nSrcY / 2][0];
                m_BufGround           [nDstX / 2][nDstY / 2][1] = stOldBufGround           [nSrcX / 2][nSrcY / 2][1];
                m_BufGround           [nDstX / 2][nDstY / 2][2] = stOldBufGround           [nSrcX / 2][nSrcY / 2][2];
                m_BufGround           [nDstX / 2][nDstY / 2][3] = stOldBufGround           [nSrcX / 2][nSrcY / 2][3];

                m_BufGroundMark       [nDstX / 2][nDstY / 2][0] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][0];
                m_BufGroundMark       [nDstX / 2][nDstY / 2][1] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][1];
                m_BufGroundMark       [nDstX / 2][nDstY / 2][2] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][2];
                m_BufGroundMark       [nDstX / 2][nDstY / 2][3] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][3];

                m_BufGroundSelectMark [nDstX / 2][nDstY / 2][0] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][0];
                m_BufGroundSelectMark [nDstX / 2][nDstY / 2][1] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][1];
                m_BufGroundSelectMark [nDstX / 2][nDstY / 2][2] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][2];
                m_BufGroundSelectMark [nDstX / 2][nDstY / 2][3] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][3];

            }
        }
    }

    m_W     = nNewW;
    m_H     = nNewH;
    m_Valid = true;

    return true;
}

int EditorMap::ObjectBlockType(int nStartX, int nStartY, int nIndex, int nSize)
{
    // assume valid map, valid parameters
    //
    // TODO
    // think about how to define *the same obj*
    // currently is:
    //  1. same animation desc
    //  2. same file index
    //  3. same image index
    //  4. same *layer*
    //  5. same alpha defination
    //
    // actually we don't have to put this check here
    // because XXXXBlockType() will be called exactly after ValidC() at the start point
    if(!ValidC(nStartX, nStartY)){ return 4; }

    if(nSize == 1){
        return ObjectValid(nStartX, nStartY, nIndex) ? 1 : 0;
    }else{
        bool bFindEmpty = false;
        bool bFindFill  = false;
        bool bFindDiff  = false;

        bool     bInited             = false;
        bool     bAlphaObjectSample  = false;
        bool     bGroundObjectSample = false;
        uint32_t nObjectSample       = 0;

        for(int nX = 0; nX < nSize; ++nX){
            for(int nY = 0; nY < nSize; ++nY){

                // this check is necessary
                if(!ValidC(nX + nStartX, nY + nStartY)){ continue; }

                if(ObjectValid(nX + nStartX, nY + nStartY, nIndex)){
                    bFindFill = true;
                    if(bInited){
                        if(nObjectSample != Object(nX + nStartX, nY + nStartY, nIndex)
                                || bGroundObjectSample != GroundObjectValid(nX + nStartX, nY + nStartY, nIndex)
                                || bAlphaObjectSample != AlphaObjectValid(nX + nStartX, nY + nStartY, nIndex)){
                            bFindDiff = true;
                        }
                    }else{
                        nObjectSample       = Object(nX + nStartX, nY + nStartY, nIndex);
                        bGroundObjectSample = GroundObjectValid(nX + nStartX, nY + nStartY, nIndex);
                        bAlphaObjectSample  = AlphaObjectValid(nX + nStartX, nY + nStartY, nIndex);
                        bInited = true;
                    }
                }else{
                    bFindEmpty = true;
                }
            }
        }

        if(bFindFill == false){
            // no information at all
            return 0;
        }else{
            // do have information
            if(bFindEmpty == false){
                // all filled
                if(bFindDiff == false){
                    // all filled and no difference exists
                    return 1;
                }else{
                    // all filled and there is difference
                    return 2;
                }
            }else{
                // there are filled and empty, combined
                return 3;
            }
        }
    }
}

int EditorMap::TileBlockType(int nStartX, int nStartY, int nSize)
{
    // assume valid map, valid parameters
    //
    // actually we don't have to put this check here
    // because XXXXBlockType() will be called exactly after ValidC() at the start point
    if(!ValidC(nStartX, nStartY) || (nStartX % 2) || (nStartY % 2)){ return 4; }

    if(nSize == 2){
        return TileValid(nStartX, nStartY) ? 1 : 0;
    }else{
        bool bFindEmpty = false;
        bool bFindFill  = false;
        bool bFindDiff  = false;

        bool bInited = false;
        uint32_t nTileSample = 0;

        for(int nX = 0; nX < nSize; ++nX){
            for(int nY = 0; nY < nSize; ++nY){

                // this check is necessary
                if(!ValidC(nX + nStartX, nY + nStartY)
                        || ((nX + nStartX) % 2)
                        || ((nY + nStartY) % 2)){
                    continue;
                }

                if(TileValid(nX + nStartX, nY + nStartY)){
                    bFindFill = true;
                    if(bInited){
                        if(nTileSample != Tile(nX + nStartX, nY + nStartY)){
                            bFindDiff = true;
                        }
                    }else{
                        nTileSample = Tile(nX + nStartX, nY + nStartY);
                        bInited = true;
                    }
                }else{
                    bFindEmpty = true;
                }
            }
        }

        if(bFindFill == false){
            // no information at all
            return 0;
        }else{
            // do have information
            if(bFindEmpty == false){
                // all filled
                if(bFindDiff == false){
                    // all filled and no difference exists
                    return 1;
                }else{
                    // all filled and there is difference
                    return 2;
                }
            }else{
                // there are filled and empty, combined
                return 3;
            }
        }
    }
}

int EditorMap::LightBlockType(int nStartX, int nStartY, int nSize)
{
    // assume valid map, valid parameters
    //
    // actually we don't have to put this check here
    // because XXXXBlockType() will be called exactly after ValidC() at the start point
    if(!ValidC(nStartX, nStartY)){ return 4; }

    if(nSize == 1){
        return LightValid(nStartX, nStartY) ? 1 : 0;
    }else{
        bool bFindEmpty = false;
        bool bFindFill  = false;
        bool bFindDiff  = false;

        bool bInited = false;
        uint16_t nLightSample = 0;

        for(int nX = 0; nX < nSize; ++nX){
            for(int nY = 0; nY < nSize; ++nY){

                // this check is necessary
                if(!ValidC(nX + nStartX, nY + nStartY)){ continue; }

                if(LightValid(nX + nStartX, nY + nStartY)){
                    bFindFill = true;
                    if(bInited){
                        if(nLightSample != Light(nX + nStartX, nY + nStartY)){
                            bFindDiff = true;
                        }
                    }else{
                        nLightSample = Light(nX + nStartX, nY + nStartY);
                        bInited = true;
                    }
                }else{
                    bFindEmpty = true;
                }
            }
        }

        if(bFindFill == false){
            // no information at all
            return 0;
        }else{
            // do have information
            if(bFindEmpty == false){
                // all filled
                if(bFindDiff == false){
                    // all filled and no difference exists
                    return 1;
                }else{
                    // all filled and there is difference
                    return 2;
                }
            }else{
                // there are filled and empty, combined
                return 3;
            }
        }
    }
}

// get block type, assume valid parameters, parameters:
//
//      nStartX                 : ..
//      nStartY                 : ..
//      nIndex                  : ignore it when nSize != 0
//      nSize                   :
//
//      return                  : define block type ->
//                                      0: no info in this block
//                                      1: there is full-filled unified info in this block
//                                      2: there is full-filled different info in this block
//                                      3: there is empty/filled combined info in this block
int EditorMap::GroundBlockType(int nStartX, int nStartY, int nIndex, int nSize)
{
    // assume valid map, valid parameters
    //
    // actually we don't have to put this check here
    // because XXXXBlockType() will be called exactly after ValidC() at the start point
    if(!ValidC(nStartX, nStartY)){ return 4; }

    if(nSize == 0){
        return GroundValid(nStartX, nStartY, nIndex) ? 1 : 0;
    }else{
        bool bFindEmpty = false;
        bool bFindFill  = false;
        bool bFindDiff  = false;

        bool bInited = false;
        uint8_t nGroundInfoSample = 0;

        for(int nX = 0; nX < nSize; ++nX){
            for(int nY = 0; nY < nSize; ++nY){

                // this check is necessary
                if(!ValidC(nX + nStartX, nY + nStartY)){ continue; }

                for(int nIdx = 0; nIdx < 4; ++nIdx){
                    if(GroundValid(nX + nStartX, nY + nStartY, nIdx)){
                        bFindFill = true;
                        if(bInited){
                            if(nGroundInfoSample != Ground(nX + nStartX, nY + nStartY, nIdx)){
                                bFindDiff = true;
                            }
                        }else{
                            nGroundInfoSample = Ground(nX + nStartX, nY + nStartY, nIdx);
                            bInited = true;
                        }
                    }else{
                        bFindEmpty = true;
                    }
                }
            }
        }

        if(bFindFill == false){
            // no information at all
            return 0;
        }else{
            // do have information
            if(bFindEmpty == false){
                // all filled
                if(bFindDiff == false){
                    // all filled and no difference exists
                    return 1;
                }else{
                    // all filled and there is difference
                    return 2;
                }
            }else{
                // there are filled and empty, combined
                return 3;
            }
        }
    }
}

// parser for ground information
// block type can be 0 1 2 3
// but the parse take 2 and 3 in the same action
//
// this is because for case 2, there is large possibility that
// a huge grid with a/b combination of a, with one exception of b
// in this case, arrange a/b combination in linear stream is bad
void EditorMap::DoCompressGround(int nX, int nY, int nSize,
        std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    if(!ValidC(nX, nY)){ return; }

    int nType = GroundBlockType(nX, nY, -1, nSize); // nSize >= 1 always
    if(nType != 0){
        // there is informaiton in this box
        stMarkV.push_back(true);
        if(nSize == 1){
            // there is info, and it's last level, end of recursion
            if(nType == 2 || nType == 3){
                // there is info, maybe a/0 or a/b combined
                // this is at last level, parser should parse it one by one
                stMarkV.push_back(true);
                for(int nIndex = 0; nIndex < 4; ++nIndex){
                    if(GroundBlockType(nX, nY, nIndex, 0) == 0){
                        // when at last level, GroundBlockType() can only return 0 / 1
                        // equivlent to GroundValid()
                        stMarkV.push_back(false);
                    }else{
                        stMarkV.push_back(true);
                        RecordGround(stDataV, nX, nY, nIndex);
                    }
                }
            }else{
                // ntype == 1 here, a/a full-filled with unified info
                stMarkV.push_back(false);
                RecordGround(stDataV, nX, nY, 0);
            }
        }else{
            // not the last level, and there is info
            if(nType == 2 || nType == 3){
                // there is info and need further parse
                stMarkV.push_back(true);
                DoCompressGround(nX            , nY            , nSize / 2, stMarkV, stDataV);
                DoCompressGround(nX + nSize / 2, nY            , nSize / 2, stMarkV, stDataV);
                DoCompressGround(nX            , nY + nSize / 2, nSize / 2, stMarkV, stDataV);
                DoCompressGround(nX + nSize / 2, nY + nSize / 2, nSize / 2, stMarkV, stDataV);
            }else{
                // nType == 1 here, unified info
                stMarkV.push_back(false);
                RecordGround(stDataV, nX, nY, 0);
            }
        }
    }else{
        // nType == 0, no information in this box
        stMarkV.push_back(false);
    }
}

void EditorMap::RecordGround(std::vector<uint8_t> &stDataV, int nX, int nY, int nIndex)
{
    stDataV.push_back(Ground(nX, nY, nIndex));
}

void EditorMap::RecordLight(std::vector<uint8_t> &stDataV, int nX, int nY)
{
    stDataV.push_back((Light(nX, nY) & 0X00FF) >> 0);
    stDataV.push_back((Light(nX, nY) & 0XFF00) >> 8);
}

void EditorMap::RecordObject(std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV, int nX, int nY, int nIndex)
{
    bool bGroundObj = (GroundObjectValid(nX, nY, nIndex) != 0);
    bool bAlphaObj  = (AlphaObjectValid(nX, nY, nIndex) != 0);
    uint32_t nObjDesc = Object(nX, nY, nIndex);

    // all extra object attributes should be put here
    stMarkV.push_back(bGroundObj);
    stMarkV.push_back(bAlphaObj);

    stDataV.push_back((uint8_t)((nObjDesc & 0XFF000000 ) >> 24));  // ObjDesc
    stDataV.push_back((uint8_t)((nObjDesc & 0X00FF0000 ) >> 16));  // FileIndex
    stDataV.push_back((uint8_t)((nObjDesc & 0X000000FF ) >>  0));  // ImageIndex Low
    stDataV.push_back((uint8_t)((nObjDesc & 0X0000FF00 ) >>  8));  // ImageIndex Hight
}

void EditorMap::RecordTile(std::vector<uint8_t> &stDataV, int nX, int nY)
{
    uint32_t nTileDesc = Tile(nX, nY);
    // stDataV.push_back((uint8_t)((nTileDesc & 0X000000FF ) >>  0));
    // stDataV.push_back((uint8_t)((nTileDesc & 0X0000FF00 ) >>  8));
    // stDataV.push_back((uint8_t)((nTileDesc & 0X00FF0000 ) >> 16));
    // stDataV.push_back((uint8_t)((nTileDesc & 0XFF000000 ) >> 24));

    stDataV.push_back(((uint8_t)((nTileDesc & 0XFF000000 ) >> 24)) | 0X80); // Desc
    stDataV.push_back(((uint8_t)((nTileDesc & 0X00FF0000 ) >> 16))       ); // FileIndex
    stDataV.push_back(((uint8_t)((nTileDesc & 0X000000FF ) >>  0))       ); // ImageIndex, low
    stDataV.push_back(((uint8_t)((nTileDesc & 0X0000FF00 ) >>  8))       ); // ImageIndex, high
}

// Light is simplest one
void EditorMap::DoCompressLight(int nX, int nY, int nSize,
        std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    if(!ValidC(nX, nY)){ return; }

    int nType = LightBlockType(nX, nY, nSize);
    if(nType != 0){
        // there is informaiton in this box
        stMarkV.push_back(true);
        if(nSize == 1){
            // there is info, and it's last level, so nType can only be 1
            // end of recursion
            RecordLight(stDataV, nX, nY);
        }else{
            // there is info, and it's not the last level
            if(nType == 2 || nType == 3){
                // there is info and need further parse
                stMarkV.push_back(true);
                DoCompressLight(nX            , nY            , nSize / 2, stMarkV, stDataV);
                DoCompressLight(nX + nSize / 2, nY            , nSize / 2, stMarkV, stDataV);
                DoCompressLight(nX            , nY + nSize / 2, nSize / 2, stMarkV, stDataV);
                DoCompressLight(nX + nSize / 2, nY + nSize / 2, nSize / 2, stMarkV, stDataV);
            }else{
                // nType == 1 here, unified info
                stMarkV.push_back(false);
                RecordLight(stDataV, nX, nY);
            }
        }
    }else{
        // nType == 0, no information in this box
        stMarkV.push_back(false);
    }
}

// tile compression is relatively simple, end-level is 2
void EditorMap::DoCompressTile(int nX, int nY, int nSize,
        std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    if(!ValidC(nX, nY)){ return; }

    int nType = TileBlockType(nX, nY, nSize);
    if(nType != 0){
        // there is informaiton in this box
        stMarkV.push_back(true);
        if(nSize == 2){
            // there is info, and it's last level, so nType can only be 1
            // end of recursion
            RecordTile(stDataV, nX, nY);
        }else{
            // there is info, and it's not the last level
            if(nType == 2 || nType == 3){
                // there is info and need further parse
                stMarkV.push_back(true);
                DoCompressTile(nX            , nY            , nSize / 2, stMarkV, stDataV);
                DoCompressTile(nX + nSize / 2, nY            , nSize / 2, stMarkV, stDataV);
                DoCompressTile(nX            , nY + nSize / 2, nSize / 2, stMarkV, stDataV);
                DoCompressTile(nX + nSize / 2, nY + nSize / 2, nSize / 2, stMarkV, stDataV);
            }else{
                // nType == 1 here, unified info
                stMarkV.push_back(false);
                RecordTile(stDataV, nX, nY);
            }
        }
    }else{
        // nType == 0, no information in this box
        stMarkV.push_back(false);
    }
}

// object compression should take care of ground/overground info in mark vect
void EditorMap::DoCompressObject(int nX, int nY, int nIndex, int nSize,
        std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    if(!ValidC(nX, nY)){ return; }

    int nType = ObjectBlockType(nX, nY, nIndex, nSize);
    if(nType != 0){
        // there is informaiton in this box
        stMarkV.push_back(true);
        if(nSize == 1){
            // there is info, and it's last level, so nType can only be 1
            // end of recursion
            RecordObject(stMarkV, stDataV, nX, nY, nIndex);
        }else{
            // there is info, and it's not the last level
            if(nType == 2 || nType == 3){
                // there is info and need further parse
                stMarkV.push_back(true);
                DoCompressObject(nX            , nY            , nIndex, nSize / 2, stMarkV, stDataV);
                DoCompressObject(nX + nSize / 2, nY            , nIndex, nSize / 2, stMarkV, stDataV);
                DoCompressObject(nX            , nY + nSize / 2, nIndex, nSize / 2, stMarkV, stDataV);
                DoCompressObject(nX + nSize / 2, nY + nSize / 2, nIndex, nSize / 2, stMarkV, stDataV);
            }else{
                // nType == 1 here, unified info
                stMarkV.push_back(false);
                RecordObject(stMarkV, stDataV, nX, nY, nIndex);
            }
        }
    }else{
        // nType == 0, no information in this box
        stMarkV.push_back(false);
    }
}

void EditorMap::CompressObject(std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV, int nIndex)
{
    stMarkV.clear();
    stDataV.clear();
    for(int nY = 0; nY < H(); nY += 8){
        for(int nX = 0; nX < W(); nX += 8){
            DoCompressObject(nX, nY, nIndex, 8, stMarkV, stDataV);
        }
    }
}

void EditorMap::CompressLight(std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    stMarkV.clear();
    stDataV.clear();
    for(int nY = 0; nY < H(); nY += 8){
        for(int nX = 0; nX < W(); nX += 8){
            DoCompressLight(nX, nY, 8, stMarkV, stDataV);
        }
    }
}

void EditorMap::CompressGround(std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    stMarkV.clear();
    stDataV.clear();
    for(int nY = 0; nY < H(); nY += 8){
        for(int nX = 0; nX < W(); nX += 8){
            DoCompressGround(nX, nY, 8, stMarkV, stDataV);
        }
    }
}

void EditorMap::CompressTile(std::vector<bool> &stMarkV, std::vector<uint8_t> &stDataV)
{
    stMarkV.clear();
    stDataV.clear();
    for(int nY = 0; nY < H(); nY += 8){
        for(int nX = 0; nX < W(); nX += 8){
            DoCompressTile(nX, nY, 8, stMarkV, stDataV);
        }
    }
}

bool EditorMap::LoadMir2Map(const char *szFullName)
{
    delete m_Mir2Map     ; m_Mir2Map      = new Mir2Map();
    delete m_Mir2xMap    ; m_Mir2xMap     = nullptr;
    delete m_Mir2xMapData; m_Mir2xMapData = nullptr;

    if(m_Mir2Map->Load(szFullName)){
        MakeBuf(m_Mir2Map->W(), m_Mir2Map->H());
        InitBuf();
    }

    delete m_Mir2Map;
    m_Mir2Map = nullptr;

    return Valid();
}

bool EditorMap::LoadMir2xMap(const char *szFullName)
{
    delete m_Mir2Map     ; m_Mir2Map      = nullptr;
    delete m_Mir2xMap    ; m_Mir2xMap     = new Mir2xMap();
    delete m_Mir2xMapData; m_Mir2xMapData = nullptr;

    if(m_Mir2xMap->Load(szFullName)){
        MakeBuf(m_Mir2xMap->W(), m_Mir2xMap->H());
        InitBuf();
    }

    delete m_Mir2xMap;
    m_Mir2xMap = nullptr;

    return Valid();
}

bool EditorMap::LoadMir2xMapData(const char *szFullName)
{
    delete m_Mir2Map     ; m_Mir2Map      = nullptr;
    delete m_Mir2xMap    ; m_Mir2xMap     = nullptr;
    delete m_Mir2xMapData; m_Mir2xMapData = new Mir2xMapData();

    if(!m_Mir2xMapData->Load(szFullName)){
        {
            if(auto fp = std::fopen("test.bin.2", "wb")){
                std::fwrite(m_Mir2xMapData->Data(), m_Mir2xMapData->Size(), 1, fp);
                std::fclose(fp);
            }
        }
        MakeBuf(m_Mir2xMapData->W(), m_Mir2xMapData->H());
        InitBuf();
    }

    delete m_Mir2xMapData;
    m_Mir2xMapData = nullptr;

    return Valid();
}

void EditorMap::Optimize()
{
    if(!Valid()){ return; }

    // try to remove some unnecessary tile/cell
    // tile
    for(int nY = 0; nY < H(); ++nY){
        for(int nX = 0; nX < W(); ++nX){
            OptimizeTile(nX, nY);
            OptimizeCell(nX, nY);
        }
    }
}

void EditorMap::OptimizeTile(int nX, int nY)
{
    // TODO
    UNUSED(nX); UNUSED(nY);
}

void EditorMap::OptimizeCell(int nX, int nY)
{
    // TODO
    UNUSED(nX); UNUSED(nY);
}

void EditorMap::ClearBuf()
{
    m_W = 0;
    m_H = 0;
    m_Valid = false;

    m_BufLight.clear();
    m_BufLightMark.clear();
    m_BufTile.clear();
    m_BufTileMark.clear();
    m_BufObj.clear();
    m_BufObjMark.clear();
    m_BufGroundObjMark.clear();
    m_BufAlphaObjMark.clear();
    m_BufGround.clear();
    m_BufGroundMark.clear();
}

bool EditorMap::InitBuf()
{
    int nW = 0;
    int nH = 0;

    m_Valid = false;

    if(m_Mir2Map && m_Mir2Map->Valid()){
        nW = m_Mir2Map->W();
        nH = m_Mir2Map->H();
    }else if(m_Mir2xMap && m_Mir2xMap->Valid()){
        nW = m_Mir2xMap->W();
        nH = m_Mir2xMap->H();
    }else if(m_Mir2xMapData && m_Mir2xMapData->Valid()){
        nW = m_Mir2xMapData->W();
        nH = m_Mir2xMapData->H();
    }else{
        return false;
    }

    for(int nY = 0; nY < nH; ++nY){
        for(int nX = 0; nX < nW; ++nX){
            if(!(nX % 2) && !(nY % 2)){
                SetBufTile(nX, nY);
            }

            SetBufLight(nX, nY);

            SetBufObj(nX, nY, 0);
            SetBufObj(nX, nY, 1);

            SetBufGround(nX, nY, 0);
            SetBufGround(nX, nY, 1);
            SetBufGround(nX, nY, 2);
            SetBufGround(nX, nY, 3);
        }
    }

    m_W     = nW;
    m_H     = nH;
    m_Valid = true;

    return true;
}

void EditorMap::MakeBuf(int nW, int nH)
{
    // make a buffer for loading new map
    // or extend / crop old map
    ClearBuf();
    if(nW == 0 || nH == 0 || nW % 2 || nH % 2){ return; }

    // m_BufLight[nX][nY]
    m_BufLight = std::vector<std::vector<uint16_t>>(nW, std::vector<uint16_t>(nH, 0));
    // m_BufLightMark[nX][nY]
    m_BufLightMark = std::vector<std::vector<int>>(nW, std::vector<int>(nH, 0));

    // m_BufTile[nX][nY]
    m_BufTile = std::vector<std::vector<uint32_t>>(nW / 2, std::vector<uint32_t>(nH / 2, 0));
    // m_BufTileMark[nX][nY]
    m_BufTileMark = std::vector<std::vector<int>>(nW / 2, std::vector<int>(nH / 2, 0));

    // m_BufObj[nX][nY][0]
    m_BufObj = std::vector<std::vector<std::array<uint32_t, 2>>>(
            nW, std::vector<std::array<uint32_t, 2>>(nH, {0, 0}));
    // m_BufObjMark[nX][nY][0]
    m_BufObjMark = std::vector<std::vector<std::array<int, 2>>>(
            nW, std::vector<std::array<int, 2>>(nH, {0, 0}));
    // m_BufGroundObjMark[nX][nY][0]
    m_BufGroundObjMark = std::vector<std::vector<std::array<int, 2>>>(
            nW, std::vector<std::array<int, 2>>(nH, {0, 0}));
    // m_BufAlphaObjMark[nX][nY][0]
    m_BufAlphaObjMark = std::vector<std::vector<std::array<int, 2>>>(
            nW, std::vector<std::array<int, 2>>(nH, {0, 0}));
    // m_BufAniObjMark[nX][nY][0]
    m_BufAniObjMark = std::vector<std::vector<std::array<int, 2>>>(
            nW, std::vector<std::array<int, 2>>(nH, {0, 0}));

    // m_BufGround[nX][nY][0]
    m_BufGround = std::vector<std::vector<std::array<uint8_t, 4>>>(
            nW, std::vector<std::array<uint8_t, 4>>(nH, {0, 0, 0, 0}));
    // m_BufGroundMark[nX][nY][0]
    m_BufGroundMark = std::vector<std::vector<std::array<int, 4>>>(
            nW, std::vector<std::array<int, 4>>(nH, {0, 0, 0, 0}));
    // m_BufGroundSelectMark[nX][nY][0]
    m_BufGroundSelectMark = std::vector<std::vector<std::array<int, 4>>>(
            nW, std::vector<std::array<int, 4>>(nH, {0, 0, 0, 0}));
}

void EditorMap::SetBufTile(int nX, int nY)
{
    if(m_Mir2Map && m_Mir2Map->Valid()){
        extern ImageDB g_ImageDB;
        if(m_Mir2Map->TileValid(nX, nY, g_ImageDB)){
            m_BufTile    [nX / 2][nY / 2] = m_Mir2Map->Tile(nX, nY);
            m_BufTileMark[nX / 2][nY / 2] = 1;
        }
    }else if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->TileValid(nX, nY)){
            m_BufTile    [nX / 2][nY / 2] = m_Mir2xMap->Tile(nX, nY);
            m_BufTileMark[nX / 2][nY / 2] = 1;
        }
    }else if(m_Mir2xMapData && m_Mir2xMapData->Valid()){
        if(m_Mir2xMapData->Tile(nX, nY).Param & 0X80000000){
            m_BufTile    [nX / 2][nY / 2] = m_Mir2xMapData->Tile(nX, nY).Param & 0X00FFFFFF;
            m_BufTileMark[nX / 2][nY / 2] = 1;
        }
    }
}

void EditorMap::SetBufGround(int nX, int nY, int nIndex)
{
    if(m_Mir2Map && m_Mir2Map->Valid()){
        // mir2 map
        if(m_Mir2Map->GroundValid(nX, nY)){
            m_BufGroundMark[nX][nY][nIndex] = 1;
            m_BufGround[nX][nY][nIndex] = 0X0000; // set by myselt
        }
    }else if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->GroundValid(nX, nY, nIndex)){
            m_BufGroundMark[nX][nY][nIndex] = 1;
            m_BufGround[nX][nY][nIndex] = m_Mir2xMap->Ground(nX, nY, nIndex);
        }
    }else if(m_Mir2xMapData && m_Mir2xMapData->Valid()){
        if(m_Mir2xMapData->Cell(nX, nY).Param & ((uint32_t)(1) << 23)){
            m_BufGroundMark[nX][nY][nIndex] = 1;
            m_BufGround[nX][nY][nIndex] = (uint8_t)((m_Mir2xMapData->Cell(nX, nY).Param & 0X007F0000) >> 16);
        }
    }
}

void EditorMap::SetBufObj(int nX, int nY, int nIndex)
{
    uint32_t nObj       = 0;
    int      nObjValid  = 0;
    int      nGroundObj = 0;
    int      nAniObj    = 0;
    int      nAlphaObj  = 0;

    if(m_Mir2Map && m_Mir2Map->Valid()){
        // mir2 map
        extern ImageDB g_ImageDB;
        if(m_Mir2Map->ObjectValid(nX, nY, nIndex, g_ImageDB)){
            nObjValid = 1;
            if(m_Mir2Map->GroundObjectValid(nX, nY, nIndex, g_ImageDB)){
                nGroundObj = 1;
            }
            if(m_Mir2Map->AniObjectValid(nX, nY, nIndex, g_ImageDB)){
                nAniObj = 1;
            }
            nObj = m_Mir2Map->Object(nX, nY, nIndex);
            // for Mir2Map, Object is arranged in different bit-order

            if(nAniObj == 1){
                nAlphaObj  = ((nObj & 0X80000000) ? 1 : 0);
                nObj      |= 0X80000000;
            }else{
                nAlphaObj  = 0;
                nObj      &= 0X00FFFFFF;
            }
        }
    }else if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->ObjectValid(nX, nY, nIndex)){
            nObjValid = 1;
            if(m_Mir2xMap->GroundObjectValid(nX, nY, nIndex)){
                nGroundObj = 1;
            }
            if(m_Mir2xMap->AniObjectValid(nX, nY, nIndex)){
                nAniObj = 1;
            }
            nAlphaObj = m_Mir2xMap->AlphaObjectValid(nX, nY, nIndex);
            nObj      = m_Mir2xMap->Object(nX, nY, nIndex);
        }
    }else if(m_Mir2xMapData && m_Mir2xMapData->Valid()){
        if((m_Mir2xMapData->Cell(nX, nY).Param & 0X80000000) && (m_Mir2xMapData->Cell(nX, nY).Obj[nIndex].Param & 0X80000000)){
            nObjValid = 1;
            if(m_Mir2xMapData->Cell(nX, nY).ObjParam & ((uint32_t)(1) << (nIndex ? 22 : 6))){
                nGroundObj = 1;
            }
            if(m_Mir2xMapData->Cell(nX, nY).ObjParam & ((uint32_t)(1) << (nIndex ? 23 : 7))){
                nAlphaObj = 1;
            }
            if(m_Mir2xMapData->Cell(nX, nY).ObjParam & ((uint32_t)(1) << (nIndex ? 31 : 15))){
                nAniObj = 1;
            }

            // TODO
            // currently we have only 8 + 16 bits for texture
            nObj  = (m_Mir2xMapData->Cell(nX, nY).Obj[nIndex].Param & 0X00FFFFFF);
            nObj |= (m_Mir2xMapData->Cell(nX, nY).ObjParam & (nIndex ? 0X7F000000 : 0X00007F00));
        }
    }


    m_BufObj          [nX][nY][nIndex] = nObj;
    m_BufObjMark      [nX][nY][nIndex] = nObjValid;
    m_BufGroundObjMark[nX][nY][nIndex] = nGroundObj;
    m_BufAniObjMark   [nX][nY][nIndex] = nAniObj;
    m_BufAlphaObjMark [nX][nY][nIndex] = nAlphaObj;
}

void EditorMap::SetBufLight(int nX, int nY)
{
    if(m_Mir2Map && m_Mir2Map->Valid()){
        // mir2 map
        if(m_Mir2Map->LightValid(nX, nY)){
            // we deprecate it
            // uint16_t nLight = m_Mir2Map->Light(nX, nY);
            //
            // make light frog by myself
            uint16_t nColorIndex = 128;  // 0, 1, 2, 3, ..., 15  4 bits
            uint16_t nAlphaIndex =   2;  // 0, 1, 2, 3           2 bits
            uint16_t nSizeType   =   0;  // 0, 1, 2, 3, ...,  7  3 bits
            uint16_t nUnused     =   0;  //                      7 bits

            UNUSED(nUnused);

            m_BufLight[nX][nY] = ((nSizeType & 0X0007) << 7) + ((nAlphaIndex & 0X0003) << 4) + ((nColorIndex & 0X000F));
            m_BufLightMark[nX][nY] = 1;
        }
    }else if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->LightValid(nX, nY)){
            m_BufLight[nX][nY]     = m_Mir2xMap->Light(nX, nY);
            m_BufLightMark[nX][nY] = 1;
        }
    }else if(m_Mir2xMapData && m_Mir2xMapData->Valid()){
        if(m_Mir2xMapData->Cell(nX, nY).Param & 0X00008000){
            m_BufLight[nX][nY]     = 0;
            m_BufLightMark[nX][nY] = 1;
        }
    }
}

std::string EditorMap::MapInfo()
{
    char szInfo[128];
    std::string szRes;

    std::sprintf(szInfo, "Width   : %d", W());
    szRes += szInfo;
    szRes += "\n";

    std::sprintf(szInfo, "Height  : %d", H());
    szRes += szInfo;
    szRes += "\n";

    szRes += "Version : 0.01";

    return szRes;
}

void EditorMap::AddSelectPoint(int nX, int nY)
{
    if(ValidP(nX, nY)){
        if(!m_SelectPointV.empty()
                && nX != m_SelectPointV.back().first
                && nY != m_SelectPointV.back().second){
            m_SelectPointV.emplace_back(nX, nY);
        }
    }
}

void EditorMap::DrawSelectPoint(std::function<void(const std::vector<std::pair<int, int>> &)> fnDrawSelect)
{
    fnDrawSelect(m_SelectPointV);
}

void EditorMap::DrawSelectGround(int nX, int nY, int nW, int nH,
        std::function<void(int, int, int)> fnDrawSelectGround)
{
    for(int nTX = nX; nTX < nX + nW; ++nTX){
        for(int nTY = nY; nTY < nY + nH; ++nTY){
            for(int nIndex = 0; nIndex < 4; ++nIndex){
                if(ValidC(nTX, nTY) && m_BufGroundSelectMark[nTX][nTY][nIndex]){
                    fnDrawSelectGround(nX, nY, nIndex);
                }
            }
        }
    }
}

void EditorMap::ClearGroundSelect()
{
    for(int nX = 0; nX < W(); ++nX){
        for(int nY = 0; nY < H(); ++nY){
            for(int nIndex = 0; nIndex < 4; ++nIndex){
                SetGroundSelect(nX, nY, nIndex, 0);
            }
        }
    }
}

void EditorMap::SetGroundSelect(int nX, int nY, int nIndex, int nSelect)
{
    m_BufGroundSelectMark[nX][nY][nIndex] = nSelect;
}

bool EditorMap::SaveMir2xMap(const char *szFullName)
{
    if(!Valid()){
        fl_alert("%s", "Invalid map!");
        return false;
    }

    auto pFile = fopen(szFullName, "wb");
    if(pFile == nullptr){
        fl_alert("Fail to open %s for writing!", szFullName);
        fclose(pFile);
        return false;
    }

    std::vector<bool>    stMarkV;
    std::vector<uint8_t> stDataV;
    std::vector<uint8_t> stOutV;

    // header, w and then h
    {
        stOutV.push_back((uint8_t)((m_W & 0X00FF)     ));
        stOutV.push_back((uint8_t)((m_W & 0XFF00) >> 8));
        stOutV.push_back((uint8_t)((m_H & 0X00FF)     ));
        stOutV.push_back((uint8_t)((m_H & 0XFF00) >> 8));
        stOutV.push_back((uint8_t)(0));
    }

    // ground information
    {
        stMarkV.clear();
        stDataV.clear();
        CompressGround(stMarkV, stDataV);
        PushData(stMarkV, stDataV, stOutV);
    }

    // light information
    {
        stMarkV.clear();
        stDataV.clear();
        CompressLight(stMarkV, stDataV);
        PushData(stMarkV, stDataV, stOutV);
    }

    // tile information
    {
        stMarkV.clear();
        stDataV.clear();
        CompressTile(stMarkV, stDataV);
        PushData(stMarkV, stDataV, stOutV);
    }

    // object-0 information
    {
        stMarkV.clear();
        stDataV.clear();
        CompressObject(stMarkV, stDataV, 0);
        PushData(stMarkV, stDataV, stOutV);
    }

    // object-1 information
    {
        stMarkV.clear();
        stDataV.clear();
        CompressObject(stMarkV, stDataV, 1);
        PushData(stMarkV, stDataV, stOutV);
    }

    if(!stOutV.empty()){
        fwrite(&(stOutV[0]), stOutV.size() * sizeof(stOutV[0]), 1, pFile);
    }
    fclose(pFile);

    return true;
}

bool EditorMap::SaveMir2xMapData(const char *szFullName)
{
    if(!Valid()){
        fl_alert("%s", "Invalid map!");
        return false;
    }

    auto pFile = fopen(szFullName, "wb");
    if(pFile == nullptr){
        fl_alert("Fail to open %s for writing!", szFullName);
        fclose(pFile);
        return false;
    }

    Mir2xMapData stMapData;
    stMapData.Allocate(W(), H());

    for(int nX = 0; nX < W(); ++nX){
        for(int nY = 0; nY < H(); ++nY){

            // tile
            if(!(nX % 2) && !(nY % 2)){
                stMapData.Tile(nX, nY).Param = TileValid(nX, nY) ? (0X80000000 | (Tile(nX, nY) & 0X0FFFFFFF)) : 0;
            }

            // cell
            auto &rstCell = stMapData.Cell(nX, nY);
            std::memset(&rstCell, 0, sizeof(rstCell));

            // for mir2xmapdata, if light / object are valid
            // we should also set the cell valid mark
            bool bCellValid = false;

            if(LightValid(nX, nY)){
                bCellValid = true;
                rstCell.Param |= (0X00008000 | (uint32_t)((Light(nX, nY) & 0X007F) << 8));
            }

            if(CanWalk(nX, nY, 0) || CanWalk(nX, nY, 1) || CanWalk(nX, nY, 2) || CanWalk(nX, nY, 3)){
                bCellValid = true;
                rstCell.Param |= 0X00800000;
            }

            if(ObjectValid(nX, nY, 0)){
                auto nObj = Object(nX, nY, 0);
                bCellValid = true;
                rstCell.Obj[0].Param |= (0X80000000 | (nObj & 0X00FFFFFF));
                if(AniObjectValid(nX, nY, 0)){
                    rstCell.ObjParam |= ((0X80000000 | (nObj & 0X7F000000)) >> 16);
                }
                if(AlphaObjectValid(nX, nY, 0)){
                    rstCell.ObjParam |= ((uint32_t)(1) << 7);
                }
                if(GroundObjectValid(nX, nY, 0)){
                    rstCell.ObjParam |= ((uint32_t)(1) << 6);
                }
            }

            if(ObjectValid(nX, nY, 1)){
                auto nObj = Object(nX, nY, 1);
                bCellValid = true;
                rstCell.Obj[1].Param |= (0X80000000 | (nObj & 0X00FFFFFF));
                if(AniObjectValid(nX, nY, 1)){
                    rstCell.ObjParam |= (0X80000000 | (nObj & 0X7F000000));
                }
                if(AlphaObjectValid(nX, nY, 1)){
                    rstCell.ObjParam |= ((uint32_t)(1) << 23);
                }
                if(GroundObjectValid(nX, nY, 1)){
                    rstCell.ObjParam |= ((uint32_t)(1) << 22);
                }
            }

            if(bCellValid){ rstCell.Param |= 0X80000000; }
        }
    }

    {
        if(auto fp = std::fopen("test.bin.1", "wb")){
            std::fwrite(stMapData.Data(), stMapData.Size(), 1, fp);
            std::fclose(fp);
        }
    }

    return stMapData.Save(szFullName) ? false : true;
}

void EditorMap::PushBit(const std::vector<bool> &stMarkV, std::vector<uint8_t> &stOutV)
{
    // will pad by zeros
    size_t nIndex = 0;
    while(nIndex < stMarkV.size()){
        uint8_t nRes = 0X00;
        // TODO think about this
        //
        // for(int nBit = 0; nBit < 8; ++nBit){
        //     nRes = nRes * 2 + ((nIndex < stMarkV.size() && stMarkV[nIndex++]) ? 1 : 0);
        // }
        for(int nBit = 0; nBit < 8; ++nBit){
            nRes = (nRes >> 1) + ((nIndex < stMarkV.size() && stMarkV[nIndex++]) ? 0X80 : 0X00);
        }
        stOutV.push_back(nRes);
    }
}

void EditorMap::PushData(const std::vector<bool> &stMarkV,
        const std::vector<uint8_t> &stDataV, std::vector<uint8_t> &stOutV)
{
    uint32_t nMarkLen = (stMarkV.size() + 7) / 8;
    uint32_t nDataLen = (stDataV.size());
    stOutV.push_back((uint8_t)((nMarkLen & 0X000000FF) >>  0));
    stOutV.push_back((uint8_t)((nMarkLen & 0X0000FF00) >>  8));
    stOutV.push_back((uint8_t)((nMarkLen & 0X00FF0000) >> 16));
    stOutV.push_back((uint8_t)((nMarkLen & 0XFF000000) >> 24));

    stOutV.push_back((uint8_t)((nDataLen & 0X000000FF) >>  0));
    stOutV.push_back((uint8_t)((nDataLen & 0X0000FF00) >>  8));
    stOutV.push_back((uint8_t)((nDataLen & 0X00FF0000) >> 16));
    stOutV.push_back((uint8_t)((nDataLen & 0XFF000000) >> 24));

    PushBit(stMarkV, stOutV);
    stOutV.insert(stOutV.end(), stDataV.begin(), stDataV.end());
    stOutV.push_back((uint8_t)(0));
}

void EditorMap::DrawLight(int nX, int nY, int nW, int nH, std::function<void(int, int)> fnDrawLight)
{
    for(int nTX = 0; nTX < nW; ++nTX){
        for(int nTY = 0; nTY < nH; ++nTY){
            if(ValidC(nTX + nX, nTY + nY) && LightValid(nTX + nX, nTY + nY)){
                fnDrawLight(nTX + nX, nTY + nY);
            }
        }
    }
}

bool EditorMap::CoverValid(int nX, int nY, int nR)
{
    if(nR  < 0){ return false;          }
    if(nR == 0){ return ValidP(nX, nY); }

    // ok now we are taking a real cover
    int nDX0 = nX - nR;
    int nDY0 = nY - nR;
    int nDX1 = nX + nR;
    int nDY1 = nY + nR;

    if(RectangleInside(0, 0, m_W * SYS_MAPGRIDXP, m_H * SYS_MAPGRIDYP, nDX0, nDY0, nR * 2, nR * 2)){
        for(int nGY = nDY0 / SYS_MAPGRIDYP; nGY <= nDY1 / SYS_MAPGRIDYP; ++nGY){
            for(int nGX = nDX0 / SYS_MAPGRIDXP; nGX <= nDX1 / SYS_MAPGRIDXP; ++nGX){
                // get each small rectangle in (nX - nR, nY - nR, nX + nR, nY + nR)
                int nGPX0 = nGX * SYS_MAPGRIDXP;
                int nGPY0 = nGY * SYS_MAPGRIDYP;

                // check if circle overlaps with this grid
                if(!CircleRectangleOverlap(nX, nY, nR, nGPX0, nGPY0, SYS_MAPGRIDXP, SYS_MAPGRIDYP)){ continue; }

                int nGPX1 = nGX * SYS_MAPGRIDXP + SYS_MAPGRIDXP;
                int nGPY1 = nGY * SYS_MAPGRIDYP + SYS_MAPGRIDYP;

                int nGPMX = (nGPX0 + nGPX1) / 2;
                int nGPMY = (nGPY0 + nGPY1) / 2;

                if(CircleTriangleOverlap(nX, nY, nR, nGPMX, nGPMY, nGPX0, nGPY0, nGPX1, nGPY0)){ if(!CanWalk(nGX, nGY, 0)){ return false; } }
                if(CircleTriangleOverlap(nX, nY, nR, nGPMX, nGPMY, nGPX1, nGPY0, nGPX1, nGPY1)){ if(!CanWalk(nGX, nGY, 1)){ return false; } }
                if(CircleTriangleOverlap(nX, nY, nR, nGPMX, nGPMY, nGPX1, nGPY1, nGPX0, nGPY1)){ if(!CanWalk(nGX, nGY, 2)){ return false; } }
                if(CircleTriangleOverlap(nX, nY, nR, nGPMX, nGPMY, nGPX0, nGPY1, nGPX0, nGPY0)){ if(!CanWalk(nGX, nGY, 3)){ return false; } }
            }
        }

        return true;
    }

    // we require the whole cover inside the map
    return false;
}
