/*
AtlasTGA.h - the font cache's glyph atlas, as the smallest exact TGA
Copyright (C) 2026 Resonance3D contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#pragma once
#ifndef ATLASTGA_H
#define ATLASTGA_H

// Self-contained (standard headers only) so the host test
// tools/host-tests/mainui_atlas_tga_host_test.cpp can build it alone.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
Resonance3D (docs/r3d/divergences.md #152): the glyph atlas as the
smallest TGA that holds it exactly. Upstream stored it as a 32-bit BMP
(4 bytes a texel); every atlas the menu builds on the Xbox is white glyphs
with coverage in alpha (GetCharRGBA, the blur pass and
WhitenTransparentTexels leave R = G = B = 255 everywhere) or, for the
outlined console font, grey (ApplyOutline writes R = G = B), so it is
kept as:
- white-alpha: type 1 (colour-mapped), 8 bits a texel; the 256-entry,
  32-bit colour map is entry i = (255, 255, 255, i), so the texel byte is
  its alpha;
- grey-alpha: type 3 (greyscale), 16 bits a texel: grey, then alpha;
- anything else: type 2 (true colour), 32 bits a texel, as before.
The engine's TGA loader (engine/common/imagelib/img_tga.c, Image_LoadTGA)
expands all three to the same RGBA32 the BMP loader produced from the
32-bit BMP (img_bmp.c), texel for texel, and sets IMAGE_HAS_ALPHA and
IMAGE_HAS_COLOR by the same rules (alpha != 255; R, G, B not all equal),
so what a renderer receives is unchanged. ref/nv2a then stores white-
alpha at 8 bits and grey-alpha at 16 (#131) as it already did.
Rows are written in the CBMP's order, bottom row first, with the TGA
origin at the bottom (attribute bit 5 clear) - how both loaders read a
bottom-up file. Header fields are written byte by byte, little-endian, as
the format defines them.
*/
#define ATLAS_TGA_HEADER_BYTES   18
#define ATLAS_TGA_COLORMAP_BYTES ( 256 * 4 )

enum atlasTgaForm_e
{
	ATLAS_TGA_WHITE_ALPHA = 1, // TGA image type 1: colour-mapped
	ATLAS_TGA_RGBA        = 2, // TGA image type 2: true colour
	ATLAS_TGA_GREY_ALPHA  = 3, // TGA image type 3: greyscale
};

// The exact form of an RGBA32 atlas: decided on every texel.
static inline atlasTgaForm_e AtlasTGA_Classify( const uint8_t *rgba, size_t texelCount )
{
	bool white = true;

	for( size_t i = 0; i < texelCount; i++ )
	{
		const uint8_t *t = &rgba[i * 4];

		if( t[0] != t[1] || t[1] != t[2] )
			return ATLAS_TGA_RGBA;

		if( t[0] != 255 )
			white = false;
	}

	return white ? ATLAS_TGA_WHITE_ALPHA : ATLAS_TGA_GREY_ALPHA;
}

static inline size_t AtlasTGA_BytesPerTexel( atlasTgaForm_e form )
{
	switch( form )
	{
	case ATLAS_TGA_WHITE_ALPHA: return 1;
	case ATLAS_TGA_GREY_ALPHA:  return 2;
	default:                    return 4;
	}
}

static inline size_t AtlasTGA_Size( atlasTgaForm_e form, unsigned int width, unsigned int height )
{
	return ATLAS_TGA_HEADER_BYTES
		+ ( form == ATLAS_TGA_WHITE_ALPHA ? ATLAS_TGA_COLORMAP_BYTES : 0 )
		+ (size_t)width * height * AtlasTGA_BytesPerTexel( form );
}

static inline uint8_t *AtlasTGA_PutShort( uint8_t *p, unsigned int v )
{
	p[0] = (uint8_t)( v & 0xFF );
	p[1] = (uint8_t)(( v >> 8 ) & 0xFF );
	return p + 2;
}

// Writes the TGA of a width x height RGBA32 atlas (bottom row first) to
// out, which holds AtlasTGA_Size( form, width, height ) bytes.
static inline void AtlasTGA_Write( uint8_t *out, atlasTgaForm_e form, const uint8_t *rgba, unsigned int width, unsigned int height )
{
	const size_t texelCount = (size_t)width * height;
	uint8_t *p = out;

	*p++ = 0; // id_length: no image id
	*p++ = form == ATLAS_TGA_WHITE_ALPHA ? 1 : 0; // colormap_type
	*p++ = (uint8_t)form; // image_type
	p = AtlasTGA_PutShort( p, 0 ); // colormap_index
	p = AtlasTGA_PutShort( p, form == ATLAS_TGA_WHITE_ALPHA ? 256 : 0 ); // colormap_length
	*p++ = form == ATLAS_TGA_WHITE_ALPHA ? 32 : 0; // colormap_size, bits an entry
	p = AtlasTGA_PutShort( p, 0 ); // x_origin
	p = AtlasTGA_PutShort( p, 0 ); // y_origin
	p = AtlasTGA_PutShort( p, width );
	p = AtlasTGA_PutShort( p, height );
	*p++ = (uint8_t)( AtlasTGA_BytesPerTexel( form ) * 8 ); // pixel_size
	*p++ = 8; // attributes: 8 alpha bits, origin at the bottom (bit 5 clear)

	switch( form )
	{
	case ATLAS_TGA_WHITE_ALPHA:
		for( int i = 0; i < 256; i++ )
		{
			*p++ = 255; // B
			*p++ = 255; // G
			*p++ = 255; // R
			*p++ = (uint8_t)i; // A
		}
		for( size_t i = 0; i < texelCount; i++ )
			*p++ = rgba[i * 4 + 3];
		break;
	case ATLAS_TGA_GREY_ALPHA:
		for( size_t i = 0; i < texelCount; i++ )
		{
			*p++ = rgba[i * 4 + 0]; // grey (R = G = B)
			*p++ = rgba[i * 4 + 3];
		}
		break;
	default:
		// The canvas's bytes as they are, in the order upstream's BMP
		// held them: UploadGlyphsForRanges copies GetCharRGBA's R, G, B, A
		// straight into the BMP's B, G, R, A slots, so the BMP loader
		// decoded the canvas's R as blue and its B as red. A type 2 TGA
		// has the same B, G, R, A slots and its loader the same reading,
		// so a coloured atlas keeps exactly the colours it had (an R = G
		// = B atlas never reaches this form).
		memcpy( p, rgba, texelCount * 4 );
		break;
	}
}

// The byte count of a cached atlas TGA starting at p (avail bytes), or 0
// if it is not exactly one of the three forms AtlasTGA_Write produces.
// Image_LoadTGA does not check that the file holds all the texels its
// header promises, so a short file must never reach it.
static inline size_t AtlasTGA_CheckedSize( const uint8_t *p, size_t avail )
{
	if( avail < ATLAS_TGA_HEADER_BYTES )
		return 0;

	const unsigned int colormapType   = p[1];
	const unsigned int imageType      = p[2];
	const unsigned int colormapIndex  = p[3] | ( p[4] << 8 );
	const unsigned int colormapLength = p[5] | ( p[6] << 8 );
	const unsigned int colormapSize   = p[7];
	const unsigned int width          = p[12] | ( p[13] << 8 );
	const unsigned int height         = p[14] | ( p[15] << 8 );
	const unsigned int pixelSize      = p[16];
	atlasTgaForm_e form;

	if( p[0] != 0 || width == 0 || height == 0 || ( p[17] & 0x20 ))
		return 0;

	if( imageType == ATLAS_TGA_WHITE_ALPHA && colormapType == 1 && colormapIndex == 0
		&& colormapLength == 256 && colormapSize == 32 && pixelSize == 8 )
		form = ATLAS_TGA_WHITE_ALPHA;
	else if( imageType == ATLAS_TGA_GREY_ALPHA && colormapType == 0 && pixelSize == 16 )
		form = ATLAS_TGA_GREY_ALPHA;
	else if( imageType == ATLAS_TGA_RGBA && colormapType == 0 && pixelSize == 32 )
		form = ATLAS_TGA_RGBA;
	else
		return 0;

	const size_t size = AtlasTGA_Size( form, width, height );
	return size <= avail ? size : 0;
}

#endif // ATLASTGA_H
