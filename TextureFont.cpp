#include "stdafx.h"
#include "TextureFont.h"

#include <afx.h>

#include "Utils.h"

#include <fstream>
#include <cmath>

TextureFont::TextureFont(): m_bBold(false), m_bItalic(false), m_bAntiAlias(false), m_fFontSizePixels(0), m_iPadding(0),
                            m_BoundingRect()
{
	m_iCharDescent = 0;
	m_iCharLeftOverlap = 0;
	m_iCharRightOverlap = 0;
	m_iCharBaseline = 0;
	m_iCharTop = 0;
	m_iCharVertSpacing = 0;
}

TextureFont::~TextureFont()
= default;

void TextureFont::FormatFontPages()
{
	for( unsigned i = 0; i < m_apPages.size(); ++i )
		delete m_apPages[i];
	m_apPages.clear();
	for( map<unsigned int, HBITMAP>::iterator i = m_Characters.begin(); i != m_Characters.end(); ++i )
	{
		if( i->second == nullptr )
			continue;
		const int b = DeleteObject( i->second );
		ASSERT( b );
	}
	m_Characters.clear();
	
	m_sError = m_sWarnings = "";

	/*
	 * Create the system font.
	 */
	LOGFONT font;
	memset( &font, 0, sizeof(font) );
	strncpy( &font.lfFaceName[0], static_cast<const char*>(m_sFamily), 31 );
	font.lfFaceName[31] = 0;
	font.lfCharSet = DEFAULT_CHARSET;
	if( m_bBold )
		font.lfWeight = FW_BOLD;
	if( m_bItalic )
		font.lfItalic = TRUE;

//	(fPoints / 72.0f) * 90
	font.lfHeight = static_cast<LONG>(-m_fFontSizePixels);
	font.lfQuality = m_bAntiAlias? ANTIALIASED_QUALITY: NONANTIALIASED_QUALITY;
	font.lfPitchAndFamily = DEFAULT_PITCH;

	const HFONT hFont = CreateFontIndirect( &font );
	if( hFont == nullptr )
	{
		m_sError = "Font isn't available";
		return;
	}

	const HDC hDC = CreateCompatibleDC(nullptr );
	const HGDIOBJ hOldFont = SelectObject( hDC, hFont );

	/*
	 * Read high-level text metrics.
	 */
	TEXTMETRIC TextMetrics;
	GetTextMetrics( hDC, &TextMetrics );

	m_iCharBaseline = TextMetrics.tmAscent;
	m_iCharDescent = TextMetrics.tmDescent;
	m_iCharVertSpacing = TextMetrics.tmHeight + TextMetrics.tmExternalLeading;
	m_iCharTop = TextMetrics.tmInternalLeading;
	m_iCharLeftOverlap = m_iCharRightOverlap = 0;
	m_BoundingRect.top = m_BoundingRect.left = 0;
	m_BoundingRect.bottom = m_BoundingRect.right = 0;


	const int n = GetKerningPairs( hDC, 0, nullptr );
	KERNINGPAIR *kp = new KERNINGPAIR[n];
	GetKerningPairs( hDC, n, kp );

	for( int i = 0; i < n; ++i )
	{
		if( kp[i].wFirst == 'A' && kp[i].wSecond == 'A' )
			int q = 1;
		if( kp[i].wFirst == 'A' && kp[i].wSecond == 'j' )
			int q = 1;
		if( kp[i].wFirst == 'j' && kp[i].wSecond == 'j' )
			int q = 1;
	}

	m_RealBounds.clear();

	for( unsigned i = 0; i < m_PagesToGenerate.size(); ++i )
	{
		m_apPages.push_back( new FontPage );
		FormatFontPage( i, hDC );
	}

/*	OUTLINETEXTMETRIC *tm = NULL;
	int i = GetOutlineTextMetrics( hDC, 0, NULL );
	if( i )
	{
		tm = (OUTLINETEXTMETRIC *) new char[i];
		GetOutlineTextMetrics( hDC, i, tm );
	}
	delete [] tm;
*/

	SelectObject( hDC, hOldFont );
	DeleteObject( hFont );
	DeleteDC( hDC );
}

int uint_to_wstr( unsigned int c, wchar_t *str ) {
	if( c < 0x10000 ) {
		str[0] = c & 0xFFFF;
		str[1] = str[2] = 0;
		return 1;
	} else if( c < 0x110000 ) {
		const unsigned int d = c - 0x10000;
		str[0] = ((d >> 10)) & 0x3FF | 0xD800;
		str[1] = (d & 0x3FF) | 0xDC00;
		str[2] = 0;
		return 2;
	} else {
		str[0] = str[1] = str[2] = 0;
		return 0;
	}
}

void TextureFont::FormatCharacter( unsigned int c, HDC hDC )
{
	wchar_t cs[16];
	const int cl = uint_to_wstr(c, cs);
	if( cl < 1 )
		return;

	if( m_Characters.find(c) != m_Characters.end() )
		return;

	SCRIPT_ITEM *items = static_cast<SCRIPT_ITEM*>(alloca(sizeof(SCRIPT_ITEM) * 16 + 1));
	int nitems;
	if( ScriptItemize (cs, cl, 16, nullptr, nullptr, items, &nitems) != S_OK )
		return;

	int gn = 0;
	WORD gi[16];
	SCRIPT_CACHE sc = nullptr;
	WORD clusters[16];
	SCRIPT_VISATTR va[16];

	// if( GetGlyphIndicesW(hDC, cs, cl, gi, GGI_MARK_NONEXISTING_GLYPHS) && gi[0] == 0xFFFF )
		// return;
	// if( ScriptGetCMap(hDC, &sc, cs, cl, 0, gi) != S_OK || gi[1] != 0 )
		// return;
	if( ScriptShape(hDC, &sc, cs, cl, 16, &(items[0].a), gi, clusters, va, &gn) != S_OK )
		return;

/*		int ii = GetFontUnicodeRanges( hDC, NULL );
		GLYPHSET *gs = (GLYPHSET *) alloca(ii);
		GetFontUnicodeRanges( hDC, gs );

		GLYPHMETRICS gm;
		MAT2 mat;
		memset( &mat, 0, sizeof(mat) );
		mat.eM11.value = 1;
		mat.eM22.value = 1;

		ii = GetGlyphOutline( hDC, c, GGO_BEZIER, &gm, 0, NULL, &mat );
		DWORD *data = (DWORD *) alloca(ii);
		ii = GetGlyphOutline( hDC, c, GGO_BEZIER, &gm, ii, data, &mat );
*/

	if(c == L'j')
		int q = 1;

	ABC &abc = m_ABC[c];

	int adv[16];
	GOFFSET off[16];
	ABC abcs[16];

	if( ScriptPlace(hDC, &sc, gi, gn, va, &(items[0].a), adv, off, abcs) != 0 )
		return;

	// Total ABCs
	abc.abcA = abcs[0].abcA;
	abc.abcB = -abcs[0].abcA;
	abc.abcC = 0;

	int i = 0;
	for( i = 0; i < gn; i++ ) {
		abc.abcB += abcs[i].abcA;
		abc.abcB += abcs[i].abcB;
		abc.abcB += abcs[i].abcC;
		abc.abcC = abcs[i].abcC;
	}

	abc.abcB -= abc.abcC;

	// ScriptGetGlyphABCWidth( hDC, &sc, gi[0], &abc );

	if( false ) {
		/* Use GetCharABCWidthsFloatW, since it works on all types of fonts; GetCharABCWidths
		 * only works on TrueType fonts. */
		ABCFLOAT abcf;
		GetCharABCWidthsFloatW( hDC, c, c, &abcf );

		abc.abcA = lrintf( abcf.abcfA );
		abc.abcB = lrintf( abcf.abcfB );
		abc.abcC = lrintf( abcf.abcfC );
	}

	/*
	 * If the A or C widths are positive, it's simply extra space to add to either side.
	 * Move this into the B width.  The only time this actually matters is to be able to
	 * omit the A width from the first letter of a string which is left-aligned, or the
	 * last C width from a right-aligned string.  The exported fonts don't support explicit
	 * ABC widths, instead representing overhang and underhang with global m_iCharLeftOverlap
	 * and m_iCharRightOverlap settings.
	 *
	 * After making this adjustment, the glyphs' B region is left-aligned in the bitmap.
	 */
	if( abc.abcA > 0 )
	{
		abc.abcB += abc.abcA;
		abc.abcA = 0;
	}
	if( abc.abcC > 0 )
	{
		abc.abcB += abc.abcC;
		abc.abcC = 0;
	}

	/* Render the character into an empty bitmap.  Since we don't know how
	 * large the character will be, this is somewhat oversized. */
	HBITMAP hBitmap;
	{
		const HDC hTempDC = GetDC(nullptr);
		hBitmap = CreateCompatibleBitmap( hTempDC, abc.abcB, 128 );
		ReleaseDC(nullptr, hTempDC );
	}
	const HGDIOBJ hOldBitmap = SelectObject( hDC, hBitmap );

	SetTextColor( hDC, RGB(0xFF,0xFF,0xFF) );
	SetBkColor( hDC, RGB(0,0,0) );
	SetBkMode( hDC, OPAQUE );

	// TextOutW( hDC, -abc.abcA, 0, cs, cl );

	if( ScriptTextOut(hDC, &sc, -abc.abcA, 0, 0, nullptr, &(items[0].a), nullptr, 0, gi, gn, adv, nullptr, off) != 0 )
		return;

	ScriptFreeCache( &sc );

	/* Determine the real bounding box: */
	RECT &realbounds = m_RealBounds[c];
	realbounds.top = 0;
	realbounds.bottom = 10;

/*	if(c == L'j')
	{
		Surface surf;
		BitmapToSurface( hBitmap, &surf );
		GrayScaleToAlpha( &surf );

		FILE *f = fopen( "c:/foo5.png", "w+b" );
		char szErrorbuf[1024];
		SavePNG( f, szErrorbuf, &surf );
		fclose( f );
	}
*/
	{
		Surface surf;
		BitmapToSurface( hBitmap, &surf );
		GetBounds( &surf, &realbounds );
	}
	realbounds.left = 0;
	realbounds.right = abc.abcB;


/*	{
//		Graphics blit( pBitmap );
//		blit.DrawLine(&pen, abc.abcA, 5, abc.abcA+abc.abcB, 5);
		const SolidBrush solidBrush1(Color(128, 255, 0, 255));
		Pen pen( &solidBrush1, 1 );
		graphics.DrawRectangle( &pen, 0, 0, (-abc.abcA) - 1, 10 );

		pen.SetColor(Color(128, 0, 255, 255));
		graphics.DrawRectangle( &pen, 0, 0, abc.abcB - 1, 10 );

		pen.SetColor(Color(128, 255, 255, 0));
		graphics.DrawRectangle( &pen, abc.abcA + abc.abcB, 10, abc.abcC - 1, 20 );
	}
*/

	/*
	 * The bitmap is probably too big.  Resize it: remove empty space on
	 * the left, right and bottom.  Don't move it up; that'll confuse offsets.
	 */
	BitBlt( hDC, 0, 0, realbounds.right, realbounds.bottom,
		hDC, 0, 0, SRCCOPY );
	SelectObject( hDC, hOldBitmap );


	m_Characters[c] = hBitmap;

	if( realbounds.left != realbounds.right && realbounds.top != realbounds.bottom )
	{
		m_BoundingRect.top = min( m_BoundingRect.top, static_cast<LONG>(realbounds.top) );
		m_BoundingRect.left = min( m_BoundingRect.left, static_cast<LONG>(realbounds.left) );
		m_BoundingRect.right = max( m_BoundingRect.right, static_cast<LONG>(realbounds.right) );
		m_BoundingRect.bottom = max( m_BoundingRect.bottom, static_cast<LONG>(realbounds.bottom) );
		if( m_BoundingRect.left == m_BoundingRect.right && m_BoundingRect.top == m_BoundingRect.bottom )
			m_BoundingRect = realbounds;
	}

	m_iCharLeftOverlap = max( m_iCharLeftOverlap, -static_cast<int>(abc.abcA) );
	m_iCharRightOverlap = max( m_iCharRightOverlap, static_cast<int>(abc.abcC) - static_cast<int>(abc.abcB) );

//	const SolidBrush solidBrush(Color(128, 255, 0, 255));
//	Pen pen(&solidBrush, 1);
//	graphics.DrawRectangle(&pen, bounds);

//	Graphics blit( pBitmap );
//	blit.DrawLine(&pen, abc.abcA, 5, abc.abcA+abc.abcB, 5);
//	blit.DrawRectangle( &pen, 1, 10, abc.abcA, 10 );

//	pen.SetColor(Color(255, 0, 255, 0));
//	graphics.DrawRectangle(&pen, realbounds);
}

/* Return the number of pixels the characters are shifted downwards in the final
 * page for m_iPadding. */
int TextureFont::GetTopPadding() const
{
	return m_iPadding/2;
}

void TextureFont::FormatFontPage( int iPage, HDC hDC )
{
	const FontPageDescription &Desc = m_PagesToGenerate[iPage];

	/* First, generate bitmaps for all characters in this page. */
	for( unsigned i = 0; i < Desc.chars.size(); ++i )
		FormatCharacter( Desc.chars[i], hDC );

	FontPage *pPage = m_apPages[iPage];
	pPage->m_iFrameWidth = (m_BoundingRect.right - m_BoundingRect.left) + m_iPadding;
	pPage->m_iFrameHeight = (m_BoundingRect.bottom - m_BoundingRect.top) + m_iPadding;
	const int iDimensionMultiple = 4;	// TODO: This only needs to be 4 for doubleres textures.  It could be 2 otherwise and use less space
	pPage->m_iFrameWidth = static_cast<int>(ceil(pPage->m_iFrameWidth / static_cast<double>(iDimensionMultiple))) * iDimensionMultiple;
	pPage->m_iFrameHeight = static_cast<int>(ceil(pPage->m_iFrameHeight / static_cast<double>(iDimensionMultiple))) * iDimensionMultiple;

	pPage->m_iNumFramesX = (Desc.chars.size() == 78) ? 26 :
	                       (Desc.chars.size() > 256) ? 32 :
	                       (Desc.chars.size() >  64) ? 16 :
	                       (Desc.chars.size() >  16) ?  8 :
	                                                    4;
	pPage->m_iNumFramesY = static_cast<int>(ceil(static_cast<float>(Desc.chars.size()) / pPage->m_iNumFramesX));

	pPage->Create( pPage->m_iNumFramesX*pPage->m_iFrameWidth, pPage->m_iNumFramesY*pPage->m_iFrameHeight );

	const HGDIOBJ hOldBitmap = SelectObject( hDC, pPage->m_hPage );

	const HDC hSrcDC = CreateCompatibleDC(nullptr );

	int iRow = 0, iCol = 0;
	for( unsigned CurChar = 0; CurChar < Desc.chars.size(); ++CurChar )
	{
		const unsigned int c = Desc.chars[CurChar];
		const ABC &abc = m_ABC[c];
		
		/* The current frame is at fOffsetX/fOffsetY.  Center the character
		 * horizontally in the frame.  We can align it however we want
		 * vertically, as long as we align the baselines. */
		float fOffsetX = static_cast<float>(pPage->m_iFrameWidth)*iCol; /* origin -> frame top-left */
		fOffsetX += pPage->m_iFrameWidth/2.0f; /* frame top-left -> frame center */
		fOffsetX -= (abc.abcA+abc.abcB+abc.abcC)/2.0f;
		fOffsetX += abc.abcA;

		/* Truncate, so we don't blit to half a pixel: */
		fOffsetX = static_cast<float>(static_cast<int>(fOffsetX));

		float fOffsetY = static_cast<float>(pPage->m_iFrameHeight)*iRow;
		fOffsetY += GetTopPadding();

		if( m_Characters[c] != nullptr )
		{
			const HBITMAP hCharacterBitmap = m_Characters[c];
			const HGDIOBJ hOldSrcBitmap = SelectObject( hSrcDC, hCharacterBitmap );

			const RECT &realbounds = m_RealBounds[c];
			BitBlt( hDC, static_cast<int>(fOffsetX), static_cast<int>(fOffsetY),
				m_ABC[c].abcB, realbounds.bottom,
				hSrcDC, 0, 0, SRCCOPY );

			SelectObject( hSrcDC, hOldSrcBitmap );
		}

		++iCol;
		if( iCol == pPage->m_iNumFramesX )
		{
			iCol = 0;
			++iRow;
		}
	}

	DeleteDC( hSrcDC );
	SelectObject( hDC, hOldBitmap );
}

/* UTF-8 encode ch and append to out. */
void wchar_to_utf8( unsigned int ch, string &out )
{
	if( ch < 0x80 ) { out.append( 1, static_cast<char>(ch) ); return; }

	int cbytes = 0;
	if( ch < 0x800 ) cbytes = 1;
	else if( ch < 0x10000 )    cbytes = 2;
	else if( ch < 0x200000 )   cbytes = 3;
	else if( ch < 0x4000000 )  cbytes = 4;
	else cbytes = 5;

	{
		const int shift = cbytes*6;
		const int init_masks[] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
		out.append( 1, static_cast<char>(init_masks[cbytes - 1] | (ch >> shift)) );
	}

	for( int i = 0; i < cbytes; ++i )
	{
		const int shift = (cbytes-i-1)*6;
		out.append( 1, static_cast<char>(0x80 | ((ch >> shift) & 0x3F)) );
	}
}

#include <iomanip>

static bool IsNumberChar( unsigned int c )
{
	return c >= 0x0030  &&  c <= 0x0039;
}

void TextureFont::Save( const CString& sBasePath, const CString& sBitmapAppendBeforeExtension, bool bSaveMetrics, bool bSaveBitmaps, bool bExportStrokeTemplates )
{
	if( m_sError != "" )
		return;

	const CString inipath = sBasePath + ".ini";

	ofstream f;

	if( bSaveMetrics )
	{
		f.open(inipath.GetString());

		/* Write global properties: */
		f << "[common]\n";

		f << "Baseline=" << m_iCharBaseline + GetTopPadding() << "\n";
		f << "Top=" << m_iCharTop + GetTopPadding() << "\n";
		f << "LineSpacing=" << m_iCharVertSpacing << "\n";
		f << "DrawExtraPixelsLeft=" << m_iCharLeftOverlap << "\n";
		f << "DrawExtraPixelsRight=" << m_iCharRightOverlap << "\n";
		f << "AdvanceExtraPixels=0\n";
		f << "DefaultStrokeColor=#FFFFFF00\n";
	}

	for( unsigned i = 0; i < m_apPages.size(); ++i )
	{
		const FontPageDescription &desc = m_PagesToGenerate[i];
		ASSERT( m_apPages[i]->m_hPage );
		FontPage &page = *m_apPages[i];

		if( bSaveMetrics )
		{
			f << "\n" << "[" << desc.name.GetString() << "]\n";

			if ( desc.name == "main" ){
					f << "range basic-japanese=0\n";
			} else {
				const int iWidth = static_cast<int>(floor(log10f(static_cast<float>(desc.chars.size()) / page.m_iNumFramesX) + 1.0f));

				unsigned iChar = 0;
				unsigned iLine = 0;
				while( iChar < desc.chars.size() )
				{
					f << "Line "  << setw(iWidth) << iLine << "=";
					f << setw(1);
					for( int iX = 0; iX < page.m_iNumFramesX && iChar < desc.chars.size(); ++iX, ++iChar )
					{
						const unsigned int c = desc.chars[iChar];
						string sUTF8;
						wchar_to_utf8( c, sUTF8 );
						f << sUTF8.c_str();
					}
					f << "\n";
					++iLine;
				}
			}

			f << "\n";


			/* export character widths.  "numbers" page has fixed with for all number characters. */
			vector<int> viCharWidth;
			int iMaxNumberCharWidth = 0;
			for( unsigned j = 0; j < desc.chars.size(); ++j )
			{
				/* This is the total width to advance for the whole character, which is the
				 * sum of the ABC widths. */
				const unsigned int c = desc.chars[j];
				ABC &abc = m_ABC[c];
				int iCharWidth = abc.abcA + static_cast<int>(abc.abcB) + static_cast<int>(abc.abcC);
				viCharWidth.push_back( iCharWidth );

				if( IsNumberChar( c ) )
					iMaxNumberCharWidth = max( iMaxNumberCharWidth, iCharWidth );
			}
			for( unsigned j = 0; j < desc.chars.size(); ++j )
			{
				const unsigned int c = desc.chars[j];
				int iCharWidth = viCharWidth[j];
				if( desc.name == "numbers"  &&  IsNumberChar( c ) )
					iCharWidth = iMaxNumberCharWidth;
				f << j << "=" << iCharWidth << "\n";
			}
		}

		if( bSaveBitmaps )
		{
			Surface surf;
			BitmapToSurface( m_apPages[i]->m_hPage, &surf );

			GrayScaleToAlpha( &surf );

			for( int j=0; j<2; j++ )
			{
				CString sPageName = m_PagesToGenerate[i].name.GetString();
				switch( j )
				{
				case 0:
					break;
				case 1:
					if( !bExportStrokeTemplates )
						continue;
					sPageName += "-stroke";
					break;
				default:
					ASSERT(0);
				}

				CString sFile;
				sFile.Format( "%s [%s] %ix%i%s.png",
					sBasePath.GetString(),
					sPageName.GetString(),
					m_apPages[i]->m_iNumFramesX,
					m_apPages[i]->m_iNumFramesY,
					sBitmapAppendBeforeExtension.GetString() );

				FILE *f = fopen( sFile, "w+b" );
				char szErrorbuf[1024];
				SavePNG( f, szErrorbuf, &surf );
				fclose( f );
			}
		}
	}
}


FontPage::FontPage()
{ 
	m_hPage = nullptr;
	m_iFrameWidth = 0;
	m_iFrameHeight = 0;
}

FontPage::~FontPage()
{
	DeleteObject( m_hPage );
}

void FontPage::Create( unsigned width, unsigned height )
{
	DeleteObject( m_hPage );
	const HDC hDC = GetDC(nullptr);
	m_hPage = CreateCompatibleBitmap( hDC, width, height );
	ReleaseDC(nullptr, hDC );
}

/*
 * Copyright (c) 2003-2007 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
