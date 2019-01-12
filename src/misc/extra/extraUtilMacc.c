/**CFile****************************************************************

  FileName    [extraUtilMacc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Generates multiplier accumulators.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilMacc.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/vec/vecMem.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Macc_ConstMultSpecOne( FILE * pFile, int n, int nBits, int nWidth )
{
    int nTotal = nWidth+nBits;
    int Bound  = 1 << (nBits-1);
    assert( -Bound <= n && n < Bound );
    fprintf( pFile, "// %d-bit multiplier by %d-bit constant %d generated by ABC\n", nWidth, nBits, n );
    fprintf( pFile, "module mul%03d%s (\n", Abc_AbsInt(n), n < 0 ? "_neg" : "_pos" );
    fprintf( pFile, "    input  [%d:0] i,\n", nWidth-1 );
    fprintf( pFile, "    output [%d:0] o\n", nWidth-1 );
    fprintf( pFile, ");\n" );
    fprintf( pFile, "    wire [%d:0] c = %d\'h%x;\n",          nBits-1,  nBits, Abc_AbsInt(n) );
    fprintf( pFile, "    wire [%d:0] I = {{%d{i[%d]}}, i};\n", nTotal-1, nBits, nWidth-1 );
    fprintf( pFile, "    wire [%d:0] m = I * c;\n",            nTotal-1 );
    fprintf( pFile, "    wire [%d:0] t = %cm;\n",              nTotal-1, n < 0 ? '-' : ' ' );
    fprintf( pFile, "    assign o = t[%d:%d];\n",              nTotal-1, nBits );
    fprintf( pFile, "endmodule\n\n" );
}
void Macc_ConstMultSpecTest()
{
    int nBits  =  8;
    int nWidth = 16;
    int Bound  =  1 << (nBits-1);
    int i;
    char Buffer[100];
    FILE * pFile;
    for ( i = -Bound; i < Bound; i++ )
    {
        sprintf( Buffer, "const_mul//spec%03d.v", 0xFF & i );
        pFile = fopen( Buffer, "wb" );
        Macc_ConstMultSpecOne( pFile, i, nBits, nWidth );
        fclose( pFile );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Macc_ConstMultGenerate( int nBits )
{
    unsigned Mask = Abc_InfoMask( nBits );
    Vec_Wec_t * vDivs = Vec_WecStart( 2*nBits );
    Vec_Int_t * vOne, * vTwo, * vThree;
    unsigned * pPlace = ABC_CALLOC( unsigned, (1 << nBits) );
    int n, a, b, One, Two, i, k, New, Bound = 1 << (nBits-1);
    // skip trivial
    pPlace[0] = ~0;
    pPlace[1] = ~0;
    pPlace[Mask & (-1)] = ~0;
    // skip div by 2
    for ( i = 2; i < (1 << nBits); i++ )
        if ( (i & 1) == 0 )
            pPlace[i] = ~0;
    // collect trivial
    printf( "Generating numbers using %d adders:\n", 0 );
    for ( i = 0; i < nBits; i++ )
    {
        Vec_WecPush( vDivs, 0, 1<<i );
        printf( "%d = %d << %d\n", 1<<i, 1, i );
    }
    // generate for each adder count
    for ( n = 1; n < nBits; n++ )
    {
        // quit if all of them are finished
        for ( a = 1; a < (1 << nBits); a++ )
            if ( pPlace[a] == 0 )
                break;
        if ( a == (1 << nBits) )
            break;

        printf( "Generating numbers using %d adders:\n", n );
        vThree = Vec_WecEntry( vDivs, n );
        for ( a = 0; a < nBits; a++ )
        for ( b = 0; b < nBits; b++ )
        {
            if ( a + b != n-1 )
                continue;
            vOne = Vec_WecEntry( vDivs, a );
            vTwo = Vec_WecEntry( vDivs, b );
            Vec_IntForEachEntry( vOne, One, i )
            Vec_IntForEachEntry( vTwo, Two, k )
            {
                New = One + Two;
                if ( New >= -Bound && New < Bound && pPlace[Mask & New] == 0 )
                {
                    if ( New > 0 )
                        Vec_IntPush( vThree, New );

                    pPlace[Mask & New] = (One << 16) | Two;
                    printf( "%d = %d + %d\n", New, One, Two );
                }
                New = One - Two;
                if ( New >= -Bound && New < Bound && pPlace[Mask & New] == 0 )
                {
                    if ( New > 0 )
                        Vec_IntPush( vThree, New );

                    pPlace[Mask & New] = (One << 16) | Two | (1 << 15);
                    printf( "%d = %d - %d\n", New, One, Two );
                }
                New = Two - One;
                if ( New >= -Bound && New < Bound && pPlace[Mask & New] == 0 )
                {
                    if ( New > 0 )
                        Vec_IntPush( vThree, New );

                    pPlace[Mask & New] = (Two << 16) | One | (1 << 15);
                    printf( "%d = %d - %d\n", New, Two, One );
                }
            }
            printf( "Adding one incrementor:\n" );
            Vec_IntForEachEntry( vThree, One, i )
            {
                if ( One < 0 )
                    continue;
                New = -One;
                if ( pPlace[Mask & New] == 0 )
                {
                    pPlace[Mask & New] = One << 16;
                    printf( "-%d = ~%d + 1\n", One, One );
                }
            }
        }
    }
    Vec_WecPrint( vDivs, 0 );
    Vec_WecFree( vDivs );
    //ABC_FREE( pPlace );
    return pPlace;
}
void Macc_ConstMultGenTest0()
{
    int nBits  =  8;
    unsigned * p = Macc_ConstMultGenerate( nBits );
    ABC_FREE( p );
}

void Macc_ConstMultGenOne_rec( FILE * pFile, unsigned * p, int n, int nBits, int nWidth )
{
    unsigned Mask = Abc_InfoMask( nBits );
    unsigned New  = Mask & (unsigned)n;
    int nTotal    = nWidth+nBits;
    int One = p[New] >> 16;
    int Two = p[New] & 0x7FFF;
    char Sign  = n < 0 ? 'N' : 'n';
    char Oper  = ((p[New] >> 15) & 1) ? '-' : '+';
    if ( p[New] == ~0 )
    {
        // count trailing zeros
        int nn, nZeros;
        for ( nZeros = 0; nZeros < nBits; nZeros++ )
            if ( (n >> nZeros) & 1 )
                break;
        nn = n >> nZeros;
        if ( nn == -1 )
            fprintf( pFile, "    wire [%d:0] N1 = -n1;\n", nTotal-1 );
        if ( Abc_AbsInt(nn) != 1 )
            Macc_ConstMultGenOne_rec( pFile, p, nn, nBits, nWidth );
        if ( nZeros > 0 )
            fprintf( pFile, "    wire [%d:0] %c%d = %c%d << %d;\n", nTotal-1, Sign, Abc_AbsInt(n), Sign, Abc_AbsInt(nn), nZeros );
    }
    else if ( One && Two ) // add/sub
    {
        Macc_ConstMultGenOne_rec( pFile, p, One, nBits, nWidth );
        Macc_ConstMultGenOne_rec( pFile, p, Two, nBits, nWidth );
        fprintf( pFile, "    wire [%d:0] %c%d = n%d %c n%d;\n", nTotal-1, Sign, Abc_AbsInt(n), One, Oper, Two );
    }
    else if ( Two == 0 ) // minus
    {
        Macc_ConstMultGenOne_rec( pFile, p, One, nBits, nWidth );
        fprintf( pFile, "    wire [%d:0] N%d = -n%d;\n", nTotal-1, One, One );
    }
}
void Macc_ConstMultGenMult( FILE * pFile, unsigned * p, int n, int nBits, int nWidth )
{
    int nTotal = nWidth+nBits;
    int Bound  = 1 << (nBits-1);
    char Sign  = n < 0 ? 'N' : 'n';
    assert( -Bound <= n && n < Bound );
    fprintf( pFile, "// %d-bit multiplier by %d-bit constant %d generated by ABC\n", nWidth, nBits, n );
    fprintf( pFile, "module mul%03d%s (\n", Abc_AbsInt(n), n < 0 ? "_neg" : "_pos" );
    fprintf( pFile, "    input  [%d:0] i,\n", nWidth-1 );
    fprintf( pFile, "    output [%d:0] o\n", nWidth-1 );
    fprintf( pFile, ");\n" );
    if ( n == 0 )
        fprintf( pFile, "    assign o = %d\'h0;\n", nWidth );
    else 
    {
        fprintf( pFile, "    wire [%d:0] n1 = {{%d{i[%d]}}, i};\n", nTotal-1, nBits, nWidth-1 );
        Macc_ConstMultGenOne_rec( pFile, p, n, nBits, nWidth );
        fprintf( pFile, "    assign o = %c%d[%d:%d];\n", Sign, Abc_AbsInt(n), nTotal-1, nBits );
    }
    fprintf( pFile, "endmodule\n\n" );
}
void Macc_ConstMultGenMacc( FILE * pFile, unsigned * p, int n, int nBits, int nWidth )
{
    int nTotal = nWidth+nBits;
    int Bound  = 1 << (nBits-1);
    char Sign  = n < 0 ? 'N' : 'n';
    assert( -Bound <= n && n < Bound );
    fprintf( pFile, "// %d-bit multiplier-accumulator by %d-bit constant %d generated by ABC\n", nWidth, nBits, n );
    fprintf( pFile, "module macc%03d%s (\n", Abc_AbsInt(n), n < 0 ? "_neg" : "_pos" );
    fprintf( pFile, "    input  [%d:0] i,\n", nWidth-1 );
    fprintf( pFile, "    input  [%d:0] c,\n", nWidth-1 );
    fprintf( pFile, "    output [%d:0] o\n", nWidth-1 );
    fprintf( pFile, ");\n" );
    if ( n == 0 )
        fprintf( pFile, "    assign o = c;\n" );
    else 
    {
        fprintf( pFile, "    wire [%d:0] n1 = {{%d{i[%d]}}, i};\n", nTotal-1, nBits, nWidth-1 );
        Macc_ConstMultGenOne_rec( pFile, p, n, nBits, nWidth );
        fprintf( pFile, "    wire [%d:0] s = %c%d[%d:%d];\n", nWidth-1, Sign, Abc_AbsInt(n), nTotal-1, nBits );
        fprintf( pFile, "    assign o = s + c;\n" );
    }
    fprintf( pFile, "endmodule\n\n" );
}
void Macc_ConstMultGenTest()
{
    int nBits  =  8;
    int nWidth = 16;
    int Bound  =  1 << (nBits-1);
    unsigned * p = Macc_ConstMultGenerate( nBits );
    int i;
    char Buffer[100];
    FILE * pFile;
    for ( i = -Bound; i < Bound; i++ )
    {
        sprintf( Buffer, "const_mul//macc%03d.v", 0xFF & i );
        pFile = fopen( Buffer, "wb" );
        Macc_ConstMultGenMacc( pFile, p, i, nBits, nWidth );
        fclose( pFile );
    }
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
