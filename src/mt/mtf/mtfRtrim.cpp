/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtfRtrim.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfRtrim;

extern mtdModule mtdVarchar;
extern mtdModule mtdNvarchar;

static mtcName mtfRtrimFunctionName[1] = {
    { NULL, 5, (void*)"RTRIM" }
};

static IDE_RC mtfRtrimEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfRtrim = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (�� �����ڰ� �ƴ�)
    mtfRtrimFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRtrimEstimate
};

static IDE_RC mtfRtrimCalculateFor1Arg( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfRtrimCalculateFor2Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static IDE_RC mtfRtrimCalculateNcharFor1Arg( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfRtrimCalculateNcharFor2Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRtrimCalculateFor1Arg,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRtrimCalculateFor2Args,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRtrimCalculateNcharFor1Arg,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRtrimCalculateNcharFor2Args,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRtrimEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );
    sModules[1] = sModules[0];

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // PROJ-1579 NCHAR
    if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
        (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                    mtfExecuteNcharFor1Arg;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                    mtfExecuteNcharFor2Args;
        }

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdNvarchar,
                                         1,
                                         aStack[1].column->precision,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                            mtfExecuteFor1Arg;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                            mtfExecuteFor2Args;
        }

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,  // BUG-16501
                                         1,
                                         aStack[1].column->precision,
                                         0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRightTrim( const mtlModule * aLanguage,
                     UChar           * aResult,
                     UShort            aResultMaxLen,
                     UShort          * aResultLen,
                     const UChar     * aSource,
                     UShort            aSourceLen,
                     const UChar     * aTrim,
                     UShort            aTrimLen )
{
/***********************************************************************
 *
 * Description : Right Trim
 *
 * Implementation :
 *
 ***********************************************************************/
 
    UChar  * sSourceIndex;
    UChar  * sSourceFence;
    UChar  * sTrimIndex;
    UChar  * sTrimFence;
    UChar  * sLast;
    idBool   sIsSame = ID_FALSE;
    UChar    sSourceSize;
    UChar    sTrimSize;

    //--------------------------------
    // Trim ���ڿ��� ���� ���� ������ ���� ��ġ ã��
    //--------------------------------

    sSourceIndex = (UChar*) aSource;
    sSourceFence = (UChar*) aSource + aSourceLen;

    sLast = sSourceFence;
    
    while ( sSourceIndex < sSourceFence )
    {
        sSourceSize =  mtl::getOneCharSize( sSourceIndex,
                                            sSourceFence,
                                            aLanguage ); 
        sTrimIndex = (UChar*) aTrim;
        sTrimFence = (UChar*) aTrim + aTrimLen;
        
        while ( sTrimIndex < sTrimFence )
        {
            sTrimSize =  mtl::getOneCharSize( sTrimIndex,
                                              sTrimFence,
                                              aLanguage );

            sIsSame = mtc::compareOneChar( sSourceIndex,
                                           sSourceSize,
                                           sTrimIndex,
                                           sTrimSize );
            
            if ( sIsSame == ID_TRUE )
            {
                // ������ ���ڰ� �ִ� ���
                break;
            }
            else
            {
                // ���� trim ���ڷ� ����
                // TASK-3420 ���ڿ� ó�� ��å ����
                (void)aLanguage->nextCharPtr( & sTrimIndex, sTrimFence );
            }
        }
        
        if ( sIsSame == ID_FALSE )
        {
            // ������ ���ڰ� ���� ���
            sLast = sSourceIndex;
        }
        else
        {
            // nothing to do
        }
        
        // ���� source ���ڷ� ����
        // TASK-3420 ���ڿ� ó�� ��å ����
        (void)aLanguage->nextCharPtr( & sSourceIndex, sSourceFence );
    }
    
    if ( ( sIsSame == ID_TRUE ) && ( sLast == sSourceFence ) )
    {
        // Source�� ��� ���ڰ� trim ���ڿ� ���� ���
        *aResultLen = 0;
    }
    else
    {
        //--------------------------------
        // Source�� ó�� ���ں��� ���������� ���� ���� ���ڱ��� sResult�� ����
        //--------------------------------

        if ( sLast == sSourceFence )
        {
            *aResultLen = ( sLast - aSource ) + 1;
        }
        else
        {
            // TASK-3420 ���ڿ� ó�� ��å ����
            (void)aLanguage->nextCharPtr( & sLast, sSourceFence );

            *aResultLen = sLast - aSource;
        }
        
        IDE_TEST_RAISE( *aResultLen > aResultMaxLen, ERR_EXCEED_MAX );
        
        idlOS::memcpy( aResult, aSource, *aResultLen );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {        
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRightTrimFor1ByteSpace( const mtlModule * aLanguage,
                                  UChar           * aResult,
                                  UShort            aResultMaxLen,
                                  UShort          * aResultLen,
                                  UChar           * aSource,
                                  UShort            aSourceLen )
{
/***********************************************************************
 *
 * Description : Right Trim
 *     BUG-10370
 *     1byte space 0x20�� ����ϴ� charset�� ���� rtrim�� �����Ѵ�.
 *
 * Implementation :
 *
 ***********************************************************************/
 
    UChar  * sSourceStart;
    UChar  * sSourceEnd;

    IDE_ASSERT_MSG( aLanguage->id != MTL_UTF16_ID,
                    "aLanguage->id : %"ID_UINT32_FMT"\n",
                    aLanguage->id );

    if ( aSourceLen > 0 )
    {
        sSourceStart = aSource;
        sSourceEnd = aSource + aSourceLen - 1;

        for ( ; sSourceStart <= sSourceEnd; sSourceEnd-- )
        {
            if ( *sSourceEnd != *aLanguage->specialCharSet[MTL_SP_IDX] )
            {
                // space�� �ƴ� ���
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sSourceStart > sSourceEnd )
        {
            // Source�� ��� ���ڰ� Trim ������ ���
            *aResultLen = 0;
        }
        else
        {
            // Source�� ó�� ���ں��� SourceEnd���� ����
            *aResultLen = ( sSourceEnd - sSourceStart ) + 1;
        
            IDE_TEST_RAISE( *aResultLen > aResultMaxLen, ERR_EXCEED_MAX );
        
            idlOS::memcpy( aResult, sSourceStart, *aResultLen );
        }
    }
    else
    {
        *aResultLen = 0;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {        
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRightTrimFor2ByteSpace( const mtlModule * aLanguage,
                                  UChar           * aResult,
                                  UShort            aResultMaxLen,
                                  UShort          * aResultLen,
                                  UChar           * aSource,
                                  UShort            aSourceLen )
{
/***********************************************************************
 *
 * Description : Right Trim
 *     BUG-10370
 *     byte space 0x00 0x20�� ����ϴ� charset�� ���� rtrim�� �����Ѵ�.
 *
 * Implementation :
 *
 ***********************************************************************/
 
    UChar  * sSourceStart;
    UChar  * sSourceEnd;

    IDE_ASSERT_MSG( aLanguage->id == MTL_UTF16_ID,
                    "aLanguage->id : %"ID_UINT32_FMT"\n",
                    aLanguage->id );

    if ( aSourceLen > 0 )
    {
        sSourceStart = aSource;
        sSourceEnd = aSource + aSourceLen - 1;

        for ( ; sSourceStart + 1 <= sSourceEnd; sSourceEnd -= 2 )
        {
            if ( *sSourceEnd !=
                 ((mtlU16Char*)aLanguage->specialCharSet[MTL_SP_IDX])->value2 )
            {
                // space�� �ƴ� ���
                break;
            }
            else
            {
                // Nothing to do.
            }

            if ( *(sSourceEnd - 1) !=
                 ((mtlU16Char*)aLanguage->specialCharSet[MTL_SP_IDX])->value1 )
            {
                // space�� �ƴ� ���
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sSourceStart > sSourceEnd )
        {
            // Source�� ��� ���ڰ� Trim ������ ���
            *aResultLen = 0;
        }
        else
        {
            // Source�� ó�� ���ں��� SourceEnd���� ����
            *aResultLen = ( sSourceEnd - sSourceStart ) + 1;
        
            IDE_TEST_RAISE( *aResultLen > aResultMaxLen, ERR_EXCEED_MAX );
        
            idlOS::memcpy( aResult, sSourceStart, *aResultLen );
        }
    }
    else
    {
        *aResultLen = 0;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {        
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRtrimCalculateFor1Arg( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Rtrim Calculate with 1 argument
 *
 * Implementation :
 *    RTRIM( char1 )
 *
 *    aStack[0] : ���ڿ� char�� �����ʿ� ���� ' '�� ��� �����Ѵ�.
 *    aStack[1] : char1 ( �Է� ���ڿ� ) 
 *
 *    ex) RTRIM( 'AAA  ' ) ==> 'AAA'
 *
 ***********************************************************************/
    
    mtdCharType*   sResult;
    mtdCharType*   sSource;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        
        IDE_TEST( mtfRightTrimFor1ByteSpace( aStack[1].column->language,
                                             sResult->value,
                                             aStack[0].column->precision,
                                             &sResult->length,
                                             sSource->value,
                                             sSource->length )
                  != IDE_SUCCESS );

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRtrimCalculateFor2Args( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Rtrim Calculate with 1 argument
 *
 * Implementation :
 *    RTRIM( char1 )
 *
 *    aStack[0] : ���ڿ� char1�� ������ ���ں��� char2�� ���ڵ�� ���Ͽ�
 *                char1�� ���ڰ� char2�� ���ڿ� ������ �����Ѵ�. �ݺ�������
 *                char2�� ���ڿ� ��ġ�Ǵ� ���ڰ� ���������� ������ �����
 *    aStack[1] : char1 ( �Է� ���ڿ� ) 
 *
 *    ex) RTRIM( 'AAAbab', ab ) ==> 'AAA'
 *
 ***********************************************************************/
    
    mtdCharType* sResult;
    mtdCharType* sSource;
    mtdCharType* sTrim;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sTrim     = (mtdCharType*)aStack[2].value;
        
        IDE_TEST( mtfRightTrim( aStack[1].column->language,
                                sResult->value,
                                aStack[0].column->precision,
                                &sResult->length,
                                ( const UChar *)sSource->value,
                                sSource->length,
                                ( const UChar *)sTrim->value,
                                sTrim->length )
                  != IDE_SUCCESS );

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRtrimCalculateNcharFor1Arg( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Rtrim Calculate with 1 argument for NCHAR
 *
 * Implementation :
 *    RTRIM( char1 )
 *
 *    aStack[0] : ���ڿ� char�� �����ʿ� ���� ' '�� ��� �����Ѵ�.
 *    aStack[1] : char1 ( �Է� ���ڿ� ) 
 *
 *    ex) RTRIM( 'AAA  ' ) ==> 'AAA'
 *
 ***********************************************************************/
    
    mtdNcharType    * sResult;
    mtdNcharType    * sSource;
    const mtlModule * sSrcCharSet;
    UShort            sResultMaxLen;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdNcharType*)aStack[0].value;
        sSource   = (mtdNcharType*)aStack[1].value;

        sSrcCharSet = aStack[1].column->language;

        sResultMaxLen = sSrcCharSet->maxPrecision(aStack[0].column->precision);

        // ------------------------------
        // RightTrim ���� �Լ�
        // ------------------------------

        if( sSrcCharSet->id == MTL_UTF16_ID )
        {
            IDE_TEST( mtfRightTrimFor2ByteSpace( aStack[1].column->language,
                                                 sResult->value,
                                                 sResultMaxLen,
                                                 &sResult->length,
                                                 sSource->value,
                                                 sSource->length )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( mtfRightTrimFor1ByteSpace( aStack[1].column->language,
                                                 sResult->value,
                                                 sResultMaxLen,
                                                 &sResult->length,
                                                 sSource->value,
                                                 sSource->length )
                      != IDE_SUCCESS );
        }

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRtrimCalculateNcharFor2Args( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Rtrim Calculate with 1 argument for NCHAR
 *
 * Implementation :
 *    RTRIM( char1 )
 *
 *    aStack[0] : ���ڿ� char1�� ������ ���ں��� char2�� ���ڵ�� ���Ͽ�
 *                char1�� ���ڰ� char2�� ���ڿ� ������ �����Ѵ�. �ݺ�������
 *                char2�� ���ڿ� ��ġ�Ǵ� ���ڰ� ���������� ������ �����
 *    aStack[1] : char1 ( �Է� ���ڿ� ) 
 *
 *    ex) RTRIM( 'AAAbab', ab ) ==> 'AAA'
 *
 ***********************************************************************/
    
    mtdNcharType    * sResult;
    mtdNcharType    * sSource;
    mtdNcharType    * sTrim;
    const mtlModule * sSrcCharSet;
    UShort            sResultMaxLen;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdNcharType*)aStack[0].value;
        sSource = (mtdNcharType*)aStack[1].value;
        sTrim   = (mtdNcharType*)aStack[2].value;

        sSrcCharSet = aStack[1].column->language;

        sResultMaxLen = sSrcCharSet->maxPrecision(aStack[0].column->precision);

        // ------------------------------
        // RightTrim ���� �Լ�
        // ------------------------------

        IDE_TEST( mtfRightTrim( aStack[1].column->language,
                                sResult->value,
                                sResultMaxLen,
                                &sResult->length,
                                ( const UChar *)sSource->value,
                                sSource->length,
                                ( const UChar *)sTrim->value,
                                sTrim->length )
                  != IDE_SUCCESS );

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}