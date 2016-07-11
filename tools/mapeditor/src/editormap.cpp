/*
 * =====================================================================================
 *
 *       Filename: editormap.cpp
 *        Created: 02/08/2016 22:17:08
 *  Last Modified: 07/10/2016 22:57:49
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
#include "sysconst.hpp"
#include "mir2xmap.hpp"
#include "editormap.hpp"
#include "supwarning.hpp"

#include <cassert>
#include <memory.h>
#include "assert.h"
#include <cstring>
#include <functional>
#include <cstdint>
#include <algorithm>
#include <vector>
#include "savepng.hpp"
#include "filesys.hpp"
#include "mathfunc.hpp"

#include <FL/fl_ask.H>

EditorMap::EditorMap()
    : m_W(0)
    , m_H(0)
    , m_Valid(false)
    , m_OldMir2Map(nullptr)
    , m_Mir2xMap(nullptr)
{
    std::memset(m_AniTileFrame, 0, sizeof(uint8_t) * 8 * 16);
    std::memset(m_AniSaveTime, 0, sizeof(uint32_t) * 8);
}

EditorMap::~EditorMap()
{
    delete m_Mir2xMap  ; m_Mir2xMap   = nullptr;
    delete m_OldMir2Map; m_OldMir2Map = nullptr;
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

void EditorMap::DrawTile(int nCX, int nCY, int nCW,  int nCH, std::function<void(uint8_t, uint16_t, int, int)> fnDrawTile)
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

void EditorMap::ExtractOneObject(
        int nXCnt, int nYCnt, int nIndex, std::function<void(uint8_t, uint16_t, uint32_t)> fnWritePNG)
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
            // 1. we draw actor, ext, even the grid is not valid
            //    and we only draw it when drawing overground objects
            //    draw it before drawing overground objects
            if(!bGround){
                fnDrawExt(nXCnt, nYCnt);
            }

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
        }
    }
}

void EditorMap::UpdateFrame(int nLoopTime)
{
    // m_AniTileFrame[i][j]:
    //     i: denotes how fast the animation is.
    //     j: denotes how many frames the animation has.

    if(!Valid()){ return; }

    uint32_t dwDelayMS[] = {150, 200, 250, 300, 350, 400, 420, 450};

    for(int nCnt = 0; nCnt < 8; ++nCnt){
        m_AniSaveTime[nCnt] += nLoopTime;
        if(m_AniSaveTime[nCnt] > dwDelayMS[nCnt]){
            for(int nFrame = 0; nFrame < 16; ++nFrame){
                m_AniTileFrame[nCnt][nFrame]++;
                if(m_AniTileFrame[nCnt][nFrame] >= nFrame){
                    m_AniTileFrame[nCnt][nFrame] = 0;
                }
            }
            m_AniSaveTime[nCnt] = 0;
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
    auto stOldBufLightMark        = m_BufEditCellDescV2D.LightMark;
    auto stOldBufLight            = m_BufEditCellDescV2D.Light;

    auto stOldBufTileMark         = m_BufEditCellDescV2D.TileMark;
    auto stOldBufTile             = m_BufEditCellDescV2D.Tile;

    auto stOldBufObjMark          = m_BufEditCellDescV2D.ObjMark;
    auto stOldBufGroundObjMark    = m_BufEditCellDescV2D.GroundObjMark;
    auto stOldBufAlphaObjMark     = m_BufEditCellDescV2D.AlphaObjMark;
    auto stOldBufAniObjMark       = m_BufEditCellDescV2D.AniObjMark;
    auto stOldBufObj              = m_BufEditCellDescV2D.Obj;
    auto stOldBufObjGridTag       = m_BufEditCellDescV2D.ObjGridTag;

    auto stOldBufGround           = m_BufEditCellDescV2D.Ground;
    auto stOldBufGroundMark       = m_BufEditCellDescV2D.GroundMark;
    auto stOldBufGroundSelectMark = m_BufEditCellDescV2D.GroundSelectMark;

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
                    m_BufEditCellDescV2D.Tile    [nDstX / 2][nDstY / 2] = stOldBufTile    [nSrcX / 2][nSrcY / 2];
                    m_BufEditCellDescV2D.TileMark[nDstX / 2][nDstY / 2] = stOldBufTileMark[nSrcX / 2][nSrcY / 2];
                }

                // TODO bug here? why I divide by 2?
                m_BufEditCellDescV2D.Light            [nDstX / 2][nDstY / 2]    = stOldBufLight            [nSrcX / 2][nSrcY / 2]   ;
                m_BufEditCellDescV2D.LightMark        [nDstX / 2][nDstY / 2]    = stOldBufLightMark        [nSrcX / 2][nSrcY / 2]   ;

                m_BufEditCellDescV2D.Obj              [nDstX / 2][nDstY / 2][0] = stOldBufObj              [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.Obj              [nDstX / 2][nDstY / 2][1] = stOldBufObj              [nSrcX / 2][nSrcY / 2][1];

                m_BufEditCellDescV2D.ObjGridTag       [nDstX / 2][nDstY / 2][0] = stOldBufObjGridTag       [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.ObjGridTag       [nDstX / 2][nDstY / 2][1] = stOldBufObjGridTag       [nSrcX / 2][nSrcY / 2][1];

                m_BufEditCellDescV2D.ObjMark          [nDstX / 2][nDstY / 2][0] = stOldBufObjMark          [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.ObjMark          [nDstX / 2][nDstY / 2][1] = stOldBufObjMark          [nSrcX / 2][nSrcY / 2][1];

                m_BufEditCellDescV2D.GroundObjMark    [nDstX / 2][nDstY / 2][0] = stOldBufGroundObjMark    [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.GroundObjMark    [nDstX / 2][nDstY / 2][1] = stOldBufGroundObjMark    [nSrcX / 2][nSrcY / 2][1];

                m_BufEditCellDescV2D.AlphaObjMark     [nDstX / 2][nDstY / 2][0] = stOldBufAlphaObjMark     [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.AlphaObjMark     [nDstX / 2][nDstY / 2][1] = stOldBufAlphaObjMark     [nSrcX / 2][nSrcY / 2][1];

                m_BufEditCellDescV2D.AniObjMark       [nDstX / 2][nDstY / 2][0] = stOldBufAniObjMark       [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.AniObjMark       [nDstX / 2][nDstY / 2][1] = stOldBufAniObjMark       [nSrcX / 2][nSrcY / 2][1];

                m_BufEditCellDescV2D.Ground           [nDstX / 2][nDstY / 2][0] = stOldBufGround           [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.Ground           [nDstX / 2][nDstY / 2][1] = stOldBufGround           [nSrcX / 2][nSrcY / 2][1];
                m_BufEditCellDescV2D.Ground           [nDstX / 2][nDstY / 2][2] = stOldBufGround           [nSrcX / 2][nSrcY / 2][2];
                m_BufEditCellDescV2D.Ground           [nDstX / 2][nDstY / 2][3] = stOldBufGround           [nSrcX / 2][nSrcY / 2][3];

                m_BufEditCellDescV2D.GroundMark       [nDstX / 2][nDstY / 2][0] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.GroundMark       [nDstX / 2][nDstY / 2][1] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][1];
                m_BufEditCellDescV2D.GroundMark       [nDstX / 2][nDstY / 2][2] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][2];
                m_BufEditCellDescV2D.GroundMark       [nDstX / 2][nDstY / 2][3] = stOldBufGroundMark       [nSrcX / 2][nSrcY / 2][3];

                m_BufEditCellDescV2D.GroundSelectMark [nDstX / 2][nDstY / 2][0] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][0];
                m_BufEditCellDescV2D.GroundSelectMark [nDstX / 2][nDstY / 2][1] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][1];
                m_BufEditCellDescV2D.GroundSelectMark [nDstX / 2][nDstY / 2][2] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][2];
                m_BufEditCellDescV2D.GroundSelectMark [nDstX / 2][nDstY / 2][3] = stOldBufGroundSelectMark [nSrcX / 2][nSrcY / 2][3];

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

bool EditorMap::LoadMir2xMap(const char *szFullName)
{
    delete m_OldMir2Map; m_OldMir2Map = nullptr;
    delete m_Mir2xMap  ; m_Mir2xMap   = new Mir2xMap();

    if(m_Mir2xMap->Load(szFullName)){
        MakeBuf(m_Mir2xMap->W(), m_Mir2xMap->H());
        InitBuf();
    }

    delete m_Mir2xMap;
    m_Mir2xMap = nullptr;

    return Valid();
}

bool EditorMap::LoadMir2Map(const char *szFullName)
{
    delete m_OldMir2Map; m_OldMir2Map = new Mir2Map();
    delete m_Mir2xMap  ; m_Mir2xMap   = nullptr;

    if(m_OldMir2Map->Load(szFullName)){
        MakeBuf(m_OldMir2Map->W(), m_OldMir2Map->H());
        InitBuf();
    }

    delete m_OldMir2Map;
    m_OldMir2Map = nullptr;

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

    m_BufEditCellDescV2D.Light.clear();
    m_BufEditCellDescV2D.LightMark.clear();
    m_BufEditCellDescV2D.Tile.clear();
    m_BufEditCellDescV2D.TileMark.clear();
    m_BufEditCellDescV2D.Obj.clear();
    m_BufEditCellDescV2D.ObjGridTag.clear();
    m_BufEditCellDescV2D.ObjMark.clear();
    m_BufEditCellDescV2D.GroundObjMark.clear();
    m_BufEditCellDescV2D.AlphaObjMark.clear();
    m_BufEditCellDescV2D.Ground.clear();
    m_BufEditCellDescV2D.GroundMark.clear();
}

bool EditorMap::InitBuf()
{
    int nW = 0;
    int nH = 0;

    m_Valid = false;

    if(m_Mir2xMap && m_Mir2xMap->Valid()){
        nW = m_Mir2xMap->W();
        nH = m_Mir2xMap->H();
    }else if(m_OldMir2Map && m_OldMir2Map->Valid()){
        nW = m_OldMir2Map->W();
        nH = m_OldMir2Map->H();
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
    assert(!(nW == 0 || nH == 0 || nW % 2 || nH % 2));

    // m_BufEditCellDescV2D.Light[nX][nY]
    m_BufEditCellDescV2D.Light = std::vector<std::vector<uint16_t>>(nW, std::vector<uint16_t>(nH, 0));
    // m_BufEditCellDescV2D.LightMark[nX][nY]
    m_BufEditCellDescV2D.LightMark = std::vector<std::vector<int>>(nW, std::vector<int>(nH, 0));

    // m_BufEditCellDescV2D.Tile[nX][nY]
    m_BufEditCellDescV2D.Tile = std::vector<std::vector<uint32_t>>(nW / 2, std::vector<uint32_t>(nH / 2, 0));
    // m_BufEditCellDescV2D.TileMark[nX][nY]
    m_BufEditCellDescV2D.TileMark = std::vector<std::vector<int>>(nW / 2, std::vector<int>(nH / 2, 0));

    // m_BufEditCellDescV2D.Obj[nX][nY][0]
    m_BufEditCellDescV2D.Obj = std::vector<std::vector<std::array<uint32_t, 2>>>(nW, std::vector<std::array<uint32_t, 2>>(nH, {0, 0}));
    // m_BufEditCellDescV2D.ObjMark[nX][nY][0]
    m_BufEditCellDescV2D.ObjMark = std::vector<std::vector<std::array<int, 2>>>(nW, std::vector<std::array<int, 2>>(nH, {0, 0}));
    // m_BufEditCellDescV2D.ObjGridTag[nX][nY][1][xxx]
    m_BufEditCellDescV2D.ObjGridTag = std::vector<std::vector<std::array<std::vector<int>, 2>>>(nW, std::vector<std::array<std::vector<int>, 2>>());
    // m_BufEditCellDescV2D.GroundObjMark[nX][nY][0]
    m_BufEditCellDescV2D.GroundObjMark = std::vector<std::vector<std::array<int, 2>>>(nW, std::vector<std::array<int, 2>>(nH, {0, 0}));
    // m_BufEditCellDescV2D.AlphaObjMark[nX][nY][0]
    m_BufEditCellDescV2D.AlphaObjMark = std::vector<std::vector<std::array<int, 2>>>(nW, std::vector<std::array<int, 2>>(nH, {0, 0}));
    // m_BufEditCellDescV2D.AniObjMark[nX][nY][0]
    m_BufEditCellDescV2D.AniObjMark = std::vector<std::vector<std::array<int, 2>>>(nW, std::vector<std::array<int, 2>>(nH, {0, 0}));

    // m_BufEditCellDescV2D.Ground[nX][nY][0]
    m_BufEditCellDescV2D.Ground = std::vector<std::vector<std::array<uint8_t, 4>>>(nW, std::vector<std::array<uint8_t, 4>>(nH, {0, 0, 0, 0}));
    // m_BufEditCellDescV2D.GroundMark[nX][nY][0]
    m_BufEditCellDescV2D.GroundMark = std::vector<std::vector<std::array<int, 4>>>(nW, std::vector<std::array<int, 4>>(nH, {0, 0, 0, 0}));
    // m_BufEditCellDescV2D.GroundSelectMark[nX][nY][0]
    m_BufEditCellDescV2D.GroundSelectMark = std::vector<std::vector<std::array<int, 4>>>(nW, std::vector<std::array<int, 4>>(nH, {0, 0, 0, 0}));
}

void EditorMap::SetBufTile(int nX, int nY)
{
    if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->TileValid(nX, nY)){
            m_BufEditCellDescV2D.Tile    [nX / 2][nY / 2] = m_Mir2xMap->Tile(nX, nY);
            m_BufEditCellDescV2D.TileMark[nX / 2][nY / 2] = 1;
        }
    }else if(m_OldMir2Map && m_OldMir2Map->Valid()){
        extern ImageDB g_ImageDB;
        if(m_OldMir2Map->TileValid(nX, nY, g_ImageDB)){
            m_BufEditCellDescV2D.Tile    [nX / 2][nY / 2] = m_OldMir2Map->Tile(nX, nY);
            m_BufEditCellDescV2D.TileMark[nX / 2][nY / 2] = 1;
        }
    }
}

void EditorMap::SetBufGround(int nX, int nY, int nIndex)
{
    if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->GroundValid(nX, nY, nIndex)){
            m_BufEditCellDescV2D.GroundMark[nX][nY][nIndex] = 1;
            m_BufEditCellDescV2D.Ground[nX][nY][nIndex] = m_Mir2xMap->Ground(nX, nY, nIndex);
        }
    }else if(m_OldMir2Map && m_OldMir2Map->Valid()){
        // mir2 map
        if(m_OldMir2Map->GroundValid(nX, nY)){
            m_BufEditCellDescV2D.GroundMark[nX][nY][nIndex] = 1;
            m_BufEditCellDescV2D.Ground[nX][nY][nIndex] = 0X0000; // set by myselt
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

    if(m_Mir2xMap && m_Mir2xMap->Valid()){
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
    }else if(m_OldMir2Map && m_OldMir2Map->Valid()){
        // mir2 map
        extern ImageDB g_ImageDB;
        if(m_OldMir2Map->ObjectValid(nX, nY, nIndex, g_ImageDB)){
            nObjValid = 1;
            if(m_OldMir2Map->GroundObjectValid(nX, nY, nIndex, g_ImageDB)){
                nGroundObj = 1;
            }
            if(m_OldMir2Map->AniObjectValid(nX, nY, nIndex, g_ImageDB)){
                nAniObj = 1;
            }
            nObj = m_OldMir2Map->Object(nX, nY, nIndex);
            // for Mir2Map, Object is arranged in different bit-order

            if(nAniObj == 1){
                nAlphaObj  = ((nObj & 0X80000000) ? 1 : 0);
                nObj      |= 0X80000000;
            }else{
                nAlphaObj  = 0;
                nObj      &= 0X00FFFFFF;
            }
        }
    }

    m_BufEditCellDescV2D.Obj          [nX][nY][nIndex] = nObj;
    m_BufEditCellDescV2D.ObjMark      [nX][nY][nIndex] = nObjValid;
    m_BufEditCellDescV2D.GroundObjMark[nX][nY][nIndex] = nGroundObj;
    m_BufEditCellDescV2D.AniObjMark   [nX][nY][nIndex] = nAniObj;
    m_BufEditCellDescV2D.AlphaObjMark [nX][nY][nIndex] = nAlphaObj;
}

void EditorMap::SetBufLight(int nX, int nY)
{
    if(m_Mir2xMap && m_Mir2xMap->Valid()){
        // mir2x map
        if(m_Mir2xMap->LightValid(nX, nY)){
            m_BufEditCellDescV2D.Light[nX][nY]     = m_Mir2xMap->Light(nX, nY);
            m_BufEditCellDescV2D.LightMark[nX][nY] = 1;
        }
    }else if(m_OldMir2Map && m_OldMir2Map->Valid()){
        // mir2 map
        if(m_OldMir2Map->LightValid(nX, nY)){
            // we deprecate it
            // uint16_t nLight = m_OldMir2Map->Light(nX, nY);
            //
            // make light frog by myself
            uint16_t nColorIndex = 128;  // 0, 1, 2, 3, ..., 15  4 bits
            uint16_t nAlphaIndex =   2;  // 0, 1, 2, 3           2 bits
            uint16_t nSizeType   =   0;  // 0, 1, 2, 3, ...,  7  3 bits
            uint16_t nUnused     =   0;  //                      7 bits

            UNUSED(nUnused);

            m_BufEditCellDescV2D.Light[nX][nY] = ((nSizeType & 0X0007) << 7) + ((nAlphaIndex & 0X0003) << 4) + ((nColorIndex & 0X000F));
            m_BufEditCellDescV2D.LightMark[nX][nY] = 1;
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
                if(ValidC(nTX, nTY) && m_BufEditCellDescV2D.GroundSelectMark[nTX][nTY][nIndex]){
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
    m_BufEditCellDescV2D.GroundSelectMark[nX][nY][nIndex] = nSelect;
}

bool EditorMap::Save(const char *szFullName)
{
    if(!Valid()){
        fl_alert("Invalid map!");
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

bool EditorMap::LocateObject(int nX, int nY, int *pGX, int *pGY, int *pObjIndex, int nH, const std::function<int(uint32_t)> &fnObjLen)
{
    if(!ValidP(nX, nY)){ return false; }

    // by default we scan from last row
    int nScanMinY = nY / SYS_MAPGRIDYP;
    int nScanMaxY = H();
    if(nH > 0 && nH / SYS_MAPGRIDYP > 0){
        nScanMaxY = std::min(nScanMaxY, (nY + nH) / SYS_MAPGRIDYP + 10);
    }

    int nRetGY = -1;
    int nRetIndex = -1;

    int nScanX = nX / SYS_MAPGRIDXP;
    for(int nScanY = nScanMaxY; nScanY >= nScanMinY; --nScanY){
        if(!ValidC(nScanX, nScanY)){ continue; }

        int nObjLen0 = -1;
        int nObjLen1 = -1;

        if(m_BufEditCellDescV2D.ObjMark[nScanX][nScanY][0]){ nObjLen0 = fnObjLen(m_BufEditCellDescV2D.Obj[nScanX][nScanY][0]); }
        if(m_BufEditCellDescV2D.ObjMark[nScanX][nScanY][1]){ nObjLen1 = fnObjLen(m_BufEditCellDescV2D.Obj[nScanX][nScanY][1]); }

        bool bIn0 = false;
        bool bIn1 = false;

        if(nObjLen0 > 0){ bIn0 = PointInRectangle(nX, nY, nScanX * SYS_MAPGRIDXP, nScanY * SYS_MAPGRIDYP - nObjLen0, SYS_MAPGRIDXP, nObjLen0); }
        if(nObjLen1 > 0){ bIn1 = PointInRectangle(nX, nY, nScanX * SYS_MAPGRIDXP, nScanY * SYS_MAPGRIDYP - nObjLen1, SYS_MAPGRIDXP, nObjLen1); }

        if(bIn0 || bIn1){
            nRetGY = nScanY;

            if(bIn0 && bIn1){
                // if current grid has two object with exactly the same size, and the point is inside
                // just randomly pick one object, and try multiple time to get what you want
                if(nObjLen0 == nObjLen1){
                    nRetIndex = std::rand() % 2;
                }else{
                    // select the smaller one
                    nRetIndex = (nObjLen0 < nObjLen1) ? 0 : 1;
                }
            }else{
                // select the longer one
                nRetIndex = (nObjLen0 > nObjLen1) ? 0 : 1;
            }
        }
    }

    if(nRetGY >= 0){
        // we got one
        if(pGX      ){ *pGX       = nScanX;    }
        if(pGY      ){ *pGY       = nRetGY;    }
        if(pObjIndex){ *pObjIndex = nRetIndex; }

        return true;
    }

    return false;
}
