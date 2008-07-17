/*******************************************************

   CoolReader Engine C-compatible API

   lvtextfm.cpp:  Text formatter

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/lvfnt.h"
#include "../include/lvtextfm.h"
#include "../include/lvdrawbuf.h"

#ifdef __cplusplus
#include "../include/lvimg.h"
#include "../include/lvtinydom.h"
#endif

#define FRM_ALLOC_SIZE 16

formatted_line_t * lvtextAllocFormattedLine( )
{
    formatted_line_t * pline = (formatted_line_t *)malloc(sizeof(formatted_line_t));
    memset( pline, 0, sizeof(formatted_line_t) );
    return pline;
}

formatted_line_t * lvtextAllocFormattedLineCopy( formatted_word_t * words, int word_count )
{
    formatted_line_t * pline = (formatted_line_t *)malloc(sizeof(formatted_line_t));
    memset( pline, 0, sizeof(formatted_line_t) );
    lUInt32 size = (word_count + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    pline->words = (formatted_word_t*)malloc( sizeof(formatted_word_t)*(size) );
    memcpy( pline->words, words, word_count * sizeof(formatted_word_t) );
    return pline;
}

void lvtextFreeFormattedLine( formatted_line_t * pline )
{
    if (pline->words)
        free( pline->words );
    free(pline);
}

formatted_word_t * lvtextAddFormattedWord( formatted_line_t * pline )
{
    lUInt32 size = (pline->word_count + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pline->word_count >= size)
    {
        size += FRM_ALLOC_SIZE;
        pline->words = (formatted_word_t*)realloc( pline->words, sizeof(formatted_word_t)*(size) );
    }
    return &pline->words[ pline->word_count++ ];
}

formatted_line_t * lvtextAddFormattedLine( formatted_text_fragment_t * pbuffer )
{
    lUInt32 size = (pbuffer->frmlinecount + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->frmlinecount >= size)
    {
        size += FRM_ALLOC_SIZE;
        pbuffer->frmlines = (formatted_line_t**)realloc( pbuffer->frmlines, sizeof(formatted_line_t*)*(size) );
    }
    return (pbuffer->frmlines[ pbuffer->frmlinecount++ ] = lvtextAllocFormattedLine());
}

formatted_line_t * lvtextAddFormattedLineCopy( formatted_text_fragment_t * pbuffer, formatted_word_t * words, int words_count )
{
    lUInt32 size = (pbuffer->frmlinecount + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->frmlinecount >= size)
    {
        size += FRM_ALLOC_SIZE;
        pbuffer->frmlines = (formatted_line_t**)realloc( pbuffer->frmlines, sizeof(formatted_line_t*)*(size) );
    }
    return (pbuffer->frmlines[ pbuffer->frmlinecount++ ] = lvtextAllocFormattedLineCopy(words, words_count));
}

formatted_text_fragment_t * lvtextAllocFormatter( lUInt16 width )
{
    formatted_text_fragment_t * pbuffer = (formatted_text_fragment_t*)malloc( sizeof(formatted_text_fragment_t) );
    memset( pbuffer, 0, sizeof(formatted_text_fragment_t));
    pbuffer->width = width;
    return pbuffer;
}

void lvtextFreeFormatter( formatted_text_fragment_t * pbuffer )
{
    if (pbuffer->srctext)
    {
        for (lUInt32 i=0; i<pbuffer->srctextlen; i++)
        {
            if (pbuffer->srctext[i].flags & LTEXT_FLAG_OWNTEXT)
                free( (void*)pbuffer->srctext[i].t.text );
        }
        free( pbuffer->srctext );
    }
    if (pbuffer->frmlines)
    {
        for (lUInt32 i=0; i<pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( pbuffer->frmlines[i] );
        }
        free( pbuffer->frmlines );
    }
    free(pbuffer);
}

/*
   Reformats source lines stored in buffer into formatted lines
   Returns height (in pixels) of formatted text
*/
lUInt32 lvtextResize( formatted_text_fragment_t * pbuffer, int width, int page_height )
{
    if (pbuffer->frmlines)
    {
        for (lUInt32 i=0; i<pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( pbuffer->frmlines[i] );
        }
        free( pbuffer->frmlines );
    }
    pbuffer->frmlines = NULL;
    pbuffer->frmlinecount = 0;
    pbuffer->width = width;
    pbuffer->height = 0;
    pbuffer->page_height = page_height;
    return lvtextFormat( pbuffer );
}

void lvtextAddSourceLine( formatted_text_fragment_t * pbuffer,
   lvfont_handle   font,     /* handle of font to draw string */
   const lChar16 * text,     /* pointer to unicode text string */
   lUInt32         len,      /* number of chars in text, 0 for auto(strlen) */
   lUInt32         color,    /* color */
   lUInt32         bgcolor,  /* bgcolor */
   lUInt32         flags,    /* flags */
   lUInt8          interval, /* interline space, *16 (16=single, 32=double) */
   lUInt16         margin,   /* first line margin */
   void *          object,    /* pointer to custom object */
   lUInt16         offset
                         )
{
    lUInt32 srctextsize = (pbuffer->srctextlen + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->srctextlen >= srctextsize)
    {
        srctextsize += FRM_ALLOC_SIZE;
        pbuffer->srctext = (src_text_fragment_t*)realloc( pbuffer->srctext, sizeof(src_text_fragment_t)*(srctextsize) );
    }
    src_text_fragment_t * pline = &pbuffer->srctext[ pbuffer->srctextlen++ ];
    pline->t.font = font;
    if (!len) for (len=0; text[len]; len++) ;
    if (flags & LTEXT_FLAG_OWNTEXT)
    {
        /* make own copy of text */
        pline->t.text = (lChar16*)malloc( len * sizeof(lChar16) );
        memcpy((void*)pline->t.text, text, len * sizeof(lChar16));
    }
    else
    {
        pline->t.text = text;
    }
    pline->object = object;
    pline->t.len = (lUInt16)len;
    pline->margin = margin;
    pline->flags = flags;
    pline->interval = interval;
    pline->t.offset = offset;
    pline->color = color;
    pline->bgcolor = bgcolor;
}

void lvtextAddSourceObject(
   formatted_text_fragment_t * pbuffer,
   lUInt16         width,
   lUInt16         height,
   lUInt32         flags,    /* flags */
   lUInt8          interval, /* interline space, *16 (16=single, 32=double) */
   lUInt16         margin,   /* first line margin */
   void *          object    /* pointer to custom object */
                         )
{
    lUInt32 srctextsize = (pbuffer->srctextlen + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->srctextlen >= srctextsize)
    {
        srctextsize += FRM_ALLOC_SIZE;
        pbuffer->srctext = (src_text_fragment_t*)realloc( pbuffer->srctext, sizeof(src_text_fragment_t)*(srctextsize) );
    }
    src_text_fragment_t * pline = &pbuffer->srctext[ pbuffer->srctextlen++ ];
    pline->o.width = width;
    pline->o.height = height;
    pline->object = object;
    pline->margin = margin;
    pline->flags = flags | LTEXT_SRC_IS_OBJECT;
    pline->interval = interval;
}

int lvtextFinalizeLine( formatted_line_t * frmline, int width, int align,
        lUInt16 * pSrcIndex, lUInt16 * pSrcOffset )
{
    int margin = frmline->x;
    int delta = 0;
    unsigned int i;
    unsigned short w = 0;
    int expand_count = 0;
    int expand_dx, expand_dd;
    int flgRollback = 0;

    if (pSrcIndex!=NULL)
    {
        /* check whether words rollback is necessary */
        for (i=frmline->word_count-1; i>0; i--)
        {
            if ( (frmline->words[i].flags & LTEXT_WORD_CAN_BREAK_LINE_AFTER) )
                break;
            if ( (frmline->words[i].flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER) )
                break;
        }
        if (/*i > 0 && */i < frmline->word_count-1)
        {
            /* rollback */
            *pSrcIndex = frmline->words[i+1].src_text_index;
            *pSrcOffset = frmline->words[i+1].t.start;
            frmline->word_count = i+1;
            flgRollback = 1;
        }
    }


    frmline->width = 0;
    for (i=0; i<frmline->word_count; i++)
    {
        //if (i == frmline->word_count-1)
        //    w = frmline->words[i].x;
        //else
        w = frmline->words[i].width;
        frmline->words[i].x = frmline->width;
        frmline->width += w;
    }

    if (align == LTEXT_ALIGN_LEFT)
        return flgRollback;

    delta = width - frmline->width - margin;

    if (align == LTEXT_ALIGN_CENTER)
        delta /= 2;
    if ( align == LTEXT_ALIGN_CENTER || align == LTEXT_ALIGN_RIGHT )
    {
        frmline->x += delta;
    }
    else
    {
        /* LTEXT_ALIGN_WIDTH */
        for (i=0; i<frmline->word_count-1; i++)
        {
            if (frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER)
                expand_count++;
        }
        if (expand_count)
        {
            expand_dx = delta / expand_count;
            expand_dd = delta % expand_count;
            delta = 0;
            for (i=0; i<frmline->word_count-1; i++)
            {
                if (frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER)
                {
                    delta += expand_dx;
                    if (expand_dd>0)
                    {
                        delta++;
                        expand_dd--;
                    }
                }
                if ( i<frmline->word_count-1 ) {
                    frmline->words[i+1].x += delta;
                }
            }
            frmline->width = frmline->words[frmline->word_count-1].x + frmline->words[frmline->word_count-1].width;
        }
    }
    return flgRollback;
}

#define DEPRECATED_LINE_BREAK_WORD_COUNT    3
#define DEPRECATED_LINE_BREAK_SPACE_LIMIT   64

lUInt32 lvtextFormat( formatted_text_fragment_t * pbuffer )
{
    lUInt32   line_flags;
    lUInt16 * widths_buf = NULL;
    lUInt8  * flags_buf = NULL;
    lUInt32   widths_buf_size = 0;
    lUInt16   chars_measured = 0;
    lUInt16   curr_x = 0;
    lUInt16   space_left = 0;
    lUInt16   text_offset = 0;
    lUInt8    align = LTEXT_ALIGN_LEFT;
    int flgRollback = 0;
    int h, b;
    src_text_fragment_t * srcline = NULL;
    src_text_fragment_t * first_para_line = NULL;
    LVFont * font = NULL;
    formatted_line_t * frmline = NULL;
    formatted_word_t * word = NULL;
    lUInt16 i, j, last_fit, chars_left;
    lUInt16 wstart = 0;
    lUInt16 wpos = 0;
    int phase;
    int isParaStart;
    int flgObject;
    int isLinkStart;

    last_fit = 0xFFFF;
    pbuffer->height = 0;
    /* For each source line: */
    for (i = 0; i<pbuffer->srctextlen; i++)
    {
        srcline = &pbuffer->srctext[i];
        line_flags = srcline->flags;
        isLinkStart = (line_flags & LTEXT_IS_LINK) ? 1 : 0; // 1 for first word of link
        flgObject = (line_flags & LTEXT_SRC_IS_OBJECT) ? 1 : 0; // 1 for object (e.g. image)
        if (!flgObject)
        {
            font = (LVFont*)srcline->t.font;
        }
        text_offset = 0;
        if ( i==0 && !(line_flags & LTEXT_FLAG_NEWLINE) )
            line_flags |= LTEXT_ALIGN_LEFT; /* first paragraph -- left by default */
        if (line_flags & LTEXT_FLAG_NEWLINE)
            first_para_line = srcline;
        if (!flgObject && (int)widths_buf_size < (int)srcline->t.len + 64) //
        {
            widths_buf_size = srcline->t.len + 64;
            widths_buf = (lUInt16 *) realloc( widths_buf, widths_buf_size * sizeof(lUInt16) );
            flags_buf  = (lUInt8 *) realloc( flags_buf, widths_buf_size * sizeof(lUInt8) );
        }

        int textWrapped = 0;
        int wrapNextLine = 0;
        while (flgObject || srcline->t.len > text_offset)
        {
            do {

                flgRollback = 0;
                if ( !frmline )
                    frmline = lvtextAddFormattedLine( pbuffer );
                if (flgObject)
                {
                    chars_left = 1;
                    isParaStart = (line_flags & LTEXT_FLAG_NEWLINE);
                } else {
                    chars_left = srcline->t.len - text_offset;
                    isParaStart = (line_flags & LTEXT_FLAG_NEWLINE) && text_offset==0;
                }

                if ( textWrapped || !frmline || isParaStart )
                    wrapNextLine = 1;

                int line_x;
                if ( wrapNextLine ) {
                    curr_x = 0;
                    line_x = isParaStart ? srcline->margin : 0;
                } else {
                    line_x = (frmline?frmline->x:0);
                }                

                if (flgObject)
                {
                    space_left = pbuffer->width - curr_x -  line_x;
                    textWrapped = 0;
                    chars_measured = 1;
                }
                else
                {
                    space_left = pbuffer->width - curr_x -  line_x;
                    /* measure line */
                    chars_measured = font->measureText(
                        text_offset + srcline->t.text,
                        chars_left,
                        widths_buf, flags_buf,
                        space_left, //pbuffer->width,
                        '?');
                    textWrapped = (chars_measured && chars_measured<chars_left) ? 1 : 0;
                }
                phase = wrapNextLine ? 1 : 0; //
                for ( ; phase<2; ++phase)
                {
                    /* add new formatted line in phase 1 */
                    if (phase == 1)
                    {
                        if ( frmline->word_count > 0 ) {
                            flgRollback = lvtextFinalizeLine( frmline, pbuffer->width,
                                   (isParaStart && align==LTEXT_ALIGN_WIDTH)?LTEXT_ALIGN_LEFT:align,
                                   &i, &text_offset);
                            frmline = lvtextAddFormattedLine( pbuffer );
                        }
                        curr_x = 0;
                        frmline->x = isParaStart ? srcline->margin : 0;
                        space_left = pbuffer->width - curr_x - frmline->x;
                        if ( chars_left && textWrapped )
                        {
                            /* measure line */
                            chars_measured = font->measureText(
                                text_offset + srcline->t.text,
                                chars_left,
                                widths_buf, flags_buf,
                                space_left, //pbuffer->width,
                                '?');
                            textWrapped = (chars_measured && chars_measured<chars_left) ? 1 : 0;
                        }
                        if (flgRollback)
                        {
                            srcline = &pbuffer->srctext[i];
                            line_flags = srcline->flags;
                            flgObject = (line_flags & LTEXT_SRC_IS_OBJECT) ? 1 : 0;
                            font = (LVFont*)srcline->t.font;
                            for (j=i; j>0; j--)
                            {
                                if (pbuffer->srctext[j].flags & LTEXT_FLAG_NEWLINE)
                                    break;
                            }
                            first_para_line = &pbuffer->srctext[j];
                        }
                        align = (lUInt8)(first_para_line->flags & LTEXT_FLAG_NEWLINE);
                        if (!align)
                            align = LTEXT_ALIGN_LEFT;
                        if (flgRollback)
                            break;
                    }
                    last_fit = 0xFFFF;
                    if (flgObject)
                    {
                        if ( srcline->o.width <= space_left)
                        {
                            last_fit = 0;
                            break;
                        }
                        //if (phase==0)
                            continue;
                    }
                    /* try to find good place for line break */
                    for (j = 0; j<chars_left && j<chars_measured; j++)
                    {
                        if (widths_buf[j] > space_left)
                            break;
                        if (flags_buf[j] & LCHAR_ALLOW_WRAP_AFTER)
                            last_fit = j;
                        if (flags_buf[j] & LCHAR_ALLOW_HYPH_WRAP_AFTER)
                            last_fit = j;
                    }
                    if (j==chars_left && widths_buf[j-1] <= space_left)
                        last_fit = j-1;
                    if (last_fit!=0xFFFF)
                        break;
                    /* avoid using deprecated line breaks if line is filled enough */
                    if (phase==0 && space_left<pbuffer->width*DEPRECATED_LINE_BREAK_SPACE_LIMIT/256
                        && frmline->word_count>=DEPRECATED_LINE_BREAK_WORD_COUNT )
                        continue;
                    /* try to find deprecated place for line break if good is not found */
                    for (j = 0; j<chars_left && j<chars_measured; j++)
                    {
                        if (widths_buf[j] > space_left)
                            break;
                        if (flags_buf[j] & LCHAR_DEPRECATED_WRAP_AFTER)
                            last_fit = j;
                    }
                    if (last_fit!=0xFFFF)
                        break;
                    if (phase==0)
                        continue;
                    /* try to wrap in the middle of word */
                    for (j = 0; j<chars_left && j<chars_measured; j++)
                    {
                        if (widths_buf[j] > space_left)
                            break;
                    }
                    if (j)
                        last_fit = j - 1;
                    else
                        last_fit = 0;
                }
            } while (flgRollback);
            /* calc vertical align */
            //int vertical_align = pbuffer->srctext[i].flags & LTEXT_VALIGN_MASK;
            int vertical_align = srcline->flags & LTEXT_VALIGN_MASK;
            int wy = 0;
            if (!flgObject && vertical_align)
            {
                int fh = font->getHeight();
                if ( vertical_align == LTEXT_VALIGN_SUB )
                {
                    wy += fh / 2;
                }
                else if ( vertical_align == LTEXT_VALIGN_SUPER )
                {
                    wy -= fh / 2;
                }
            }
            /* add new words to frmline */
            if (flgObject)
            {
                // object
                word = lvtextAddFormattedWord( frmline );
                word->src_text_index = i;
                int scale_div = 1;
                int scale_mul = 1;
                int div_x = (srcline->o.width / pbuffer->width) + 1;
                int div_y = (srcline->o.height / pbuffer->page_height) + 1;
#if (MAX_IMAGE_SCALE_MUL==3)
                if ( srcline->o.height*3 < pbuffer->page_height-20
                        && srcline->o.width*3 < pbuffer->width - 20 )
                    scale_mul = 3;
                else
#endif
#if (MAX_IMAGE_SCALE_MUL==2) || (MAX_IMAGE_SCALE_MUL==3)
                    if ( srcline->o.height*2 < pbuffer->page_height-20
                        && srcline->o.width*2 < pbuffer->width - 20 )
                    scale_mul = 2;
                else
#endif
                if (div_x>1 || div_y>1) {
                    if (div_x>div_y)
                        scale_div = div_x;
                    else
                        scale_div = div_y;
                }
                word->o.height = srcline->o.height * scale_mul / scale_div;
                word->width = srcline->o.width * scale_mul / scale_div;
                word->flags = LTEXT_WORD_IS_OBJECT;
                word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
                word->y = 0;
                word->x = frmline->width;
                //frmline->width += word->width;
                frmline->width = word->x + word->width; //!!!
            }
            else
            {
                // text string
                word = NULL;
                for (j=0, wstart=0, wpos=0; j<=last_fit; j++)
                {
                    if (flags_buf[j] & LCHAR_IS_SPACE || j==last_fit) /* LCHAR_ALLOW_WRAP_AFTER */
                    {
                        word = lvtextAddFormattedWord( frmline );
                        word->src_text_index = i;
                        word->t.len = j - wstart + 1;
                        word->width = widths_buf[j] - wpos;
                        word->t.start = text_offset + wstart;
                        word->flags = 0;
                        if ( isLinkStart ) {
                            word->flags = LTEXT_WORD_IS_LINK_START;
                            isLinkStart = 0;
                        }
                        word->y = wy;
                        word->x = widths_buf[j] - wpos;
                        if (flags_buf[j] & LCHAR_IS_SPACE)
                            word->flags |= LTEXT_WORD_CAN_ADD_SPACE_AFTER;
                        if (flags_buf[j] & LCHAR_ALLOW_WRAP_AFTER)
                            word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
                        if (flags_buf[j] & LCHAR_ALLOW_HYPH_WRAP_AFTER)
                            word->flags |= LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER;
                        if ( text_offset+j == srcline->t.len-1 )
                        {
                            /* last char of src fragment */
                            if (i==pbuffer->srctextlen-1 || pbuffer->srctext[i+1].flags & LTEXT_FLAG_NEWLINE)
                                word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
                        }
                        //???
                        ///*
                        for (int jj=j; jj>0 && (flags_buf[jj] & LCHAR_IS_SPACE); jj--)
                            word->x = widths_buf[jj-1] - wpos;
                        //*/
                        wpos = widths_buf[j];
                        wstart = j+1;
                        frmline->width = word->x + word->width; //!!!
                    }
                }
            }
            /* update Y positions of line */
            if (flgObject)
            {
                b = word->o.height;
                h = 0;
            }
            else
            {
                if (wy!=0)
                {
                    // subscript or superscript
                    b = font->getBaseline();
                    h = font->getHeight() - b;
                }
                else
                {
                    b = (( font->getBaseline() * first_para_line->interval) >> 4);
                    h = ( ( font->getHeight() * first_para_line->interval) >> 4) - b;
                }
            }
            if ( frmline->baseline < b - wy )
                frmline->baseline = (lUInt16) (b - wy);
            if ( frmline->height < frmline->baseline + h )
                frmline->height = (lUInt16) ( frmline->baseline + h );

            if (flgObject)
            {
                curr_x += word->width;
                break;
            }
            /* skip formatted text in source line */
            text_offset += wstart;
            curr_x += wpos;
        }
    }
    /* finish line formatting */
    if (frmline)
        lvtextFinalizeLine( frmline, pbuffer->width,
        (align==LTEXT_ALIGN_WIDTH)?LTEXT_ALIGN_LEFT:align, NULL, NULL );
    /* cleanup */
    if (widths_buf)
        free(widths_buf);
    if (flags_buf)
        free(flags_buf);
    if (pbuffer->frmlinecount)
    {
        int y = 0;
        for (lUInt16 i=0; i<pbuffer->frmlinecount; i++)
        {
            pbuffer->frmlines[i]->y = y;
            y += pbuffer->frmlines[i]->height;
        }
        pbuffer->height = y;
    }
    return pbuffer->height;
}


/*
   Draws formatted text to draw buffer
*/
void lvtextDraw( formatted_text_fragment_t * text, draw_buf_t * buf, int x, int y )
{
    lUInt32 i, j;
    formatted_line_t * frmline;
    src_text_fragment_t * srcline;
    formatted_word_t * word;
    lvfont_header_t * font;
    const lChar16 * str;
    int line_y = y;
    for (i=0; i<text->frmlinecount; i++)
    {
        frmline = text->frmlines[i];
        for (j=0; j<frmline->word_count; j++)
        {
            word = &frmline->words[j];
            //int flg = 0;
            srcline = &text->srctext[word->src_text_index];
            font = (lvfont_header_t *) ( ((LVFont*)srcline->t.font)->GetHandle() );
            str = srcline->t.text + word->t.start;
            lvdrawbufDrawText( buf,
                x + frmline->x + word->x,
                line_y + (frmline->baseline - font->fontBaseline) + word->y,
                (lvfont_handle)font,
                str,
                word->t.len,
                '?' );
        }
        line_y += frmline->height;
    }
}

#ifdef __cplusplus

void LFormattedText::AddSourceObject(
            lUInt16         flags,    /* flags */
            lUInt8          interval, /* interline space, *16 (16=single, 32=double) */
            lUInt16         margin,   /* first line margin */
            void *          object    /* pointer to custom object */
     )
{
    ldomElement * node = (ldomElement*)object;
    LVImageSourceRef img = node->getObjectImageSource();
    if ( img.isNull() )
        img = LVCreateDummyImageSource( node, 50, 50 );
    lUInt16 width = (lUInt16)img->GetWidth();
    lUInt16 height = (lUInt16)img->GetHeight();
    lvtextAddSourceObject(m_pbuffer,
        width, height,
        flags, interval, margin, object );
}

class LVFormLine {
public:
    formatted_text_fragment_t * m_pbuffer;
    formatted_line_t * frmline;
    lUInt32             widths_buf_size;
    LVAutoPtr<lUInt16>  widths_buf;
    LVAutoPtr<lUInt8>   flags_buf;
    int srcIndex;
    src_text_fragment_t * srcline;
    src_text_fragment_t * first_para_line;
    int interval;
    LVFont * font;
    bool flgObject;
    bool isLinkStart;
    int text_offset;
    int line_flags;
    int wy;
    int vertical_align;
    int frmline_wrap_pos;
    int align;
    bool flgLastParaLine;
    bool flgCanBreakBeforeNextLine;
    void newLine()
    {
        frmline = lvtextAddFormattedLine( m_pbuffer );
    }

    /**
     * Set proper Y coordinate of word
     */
    void updateY(formatted_word_t * word)
    {
        /* update Y positions of line */
        int b;
        int h;
        if (flgObject)
        {
            b = word->o.height;
            h = 0;
        }
        else
        {
            LVFont * font = (LVFont*) m_pbuffer->srctext[word->src_text_index].t.font;
            if (word->y!=0)
            {
                // subscript or superscript
                b = font->getBaseline();
                h = font->getHeight() - b;
            }
            else
            {
                b = (( font->getBaseline() * interval) >> 4);
                h = ( ( font->getHeight() * interval) >> 4) - b;
            }
        }
        if ( frmline->baseline < b - word->y )
            frmline->baseline = (lUInt16) (b - word->y);
        if ( frmline->height < frmline->baseline + h )
            frmline->height = (lUInt16) ( frmline->baseline + h );
    }

    /**
     * Update current formatted line word wrap position
     * @param wordsToCommit is max number of words to commit, -1 == commit all words
     * @returns frmline word wrap position (index of word to be wrapped)
     */
    int updateWrapPos( int wordsToCommit=-1 )
    {
        if ( wordsToCommit<0 )
            wordsToCommit = frmline->word_count; // by default, commit maximum available number of words
        else if ( wordsToCommit==0 )
            wordsToCommit = 1; // at least commit one word
        // search for available wrap position
        for ( frmline_wrap_pos = wordsToCommit-1; frmline_wrap_pos>0; frmline_wrap_pos-- ) {
            if ( (frmline->words[frmline_wrap_pos].flags & LTEXT_WORD_CAN_BREAK_LINE_AFTER) )
                break;
            if ( (frmline->words[frmline_wrap_pos].flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER) )
                break;
        }
        return frmline_wrap_pos + 1;
    }

    /**
     * save part of words to final formatted line
     * @param wordsToCommit is max number of words to commit, -1 == commit all words
     * @returns frmline word wrap position (index of word to be wrapped)
     */
    void commit( int wordsToCommit=-1 )
    {
        if ( !frmline->word_count )
            return; // empty line, nothing to do
        // current src line is finished
        bool srcFinished = !srcline || flgObject || text_offset>=(int)(srcline->t.offset+srcline->t.len);
        // create new formatted line, if not last
        bool createNewLine = (srcIndex < (int)m_pbuffer->srctextlen) || !srcFinished;
        // search for line break
        int wordCount = updateWrapPos( wordsToCommit );

        if ( wordCount>0 && (int)frmline->word_count >= wordCount ) {
            formatted_line_t * newline = NULL;
            int extraWords = 0;
            if ( createNewLine ) {
                extraWords = frmline->word_count - wordCount;
                if ( extraWords > 0 ) {
                    // copy rest of words to new line
                    newline = lvtextAddFormattedLineCopy( m_pbuffer, frmline->words + wordCount, extraWords );
                } else {
                    // empty line: no words wrapped
                    newline = lvtextAddFormattedLine( m_pbuffer );
                }
            }
            frmline->word_count = wordCount;
            frmline->width = 0;
            int i;
            for ( i=0; i<wordCount; i++ ) {
                formatted_word_t * word = &frmline->words[i];
                // update X for each word
                word->x = frmline->width;
                if ( i== wordCount-1 )
                    frmline->width += word->width;
                else
                    frmline->width += word->inline_width;
                // update Y of each word
                updateY( word );
            }

            // don't spread last line of paragraph
            int nalign = align;
            if ( nalign==LTEXT_ALIGN_WIDTH && !extraWords && srcFinished )
                nalign = LTEXT_ALIGN_LEFT;

            int width = m_pbuffer->width;
            int delta = width - frmline->width - frmline->x;

            // horizontal alignment
            if (nalign == LTEXT_ALIGN_CENTER)
                delta /= 2;
            if ( nalign == LTEXT_ALIGN_CENTER || nalign == LTEXT_ALIGN_RIGHT ) {
                frmline->x += delta;
            } else if (nalign == LTEXT_ALIGN_WIDTH) {
                // spread to fill width
                int expand_count = 0;
                for (i=0; i<(int)frmline->word_count-1; i++)
                {
                    if (frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER)
                        expand_count++;
                }
                if (expand_count)
                {
                    int expand_dx = delta / expand_count;
                    int expand_dd = delta % expand_count;
                    delta = 0;
                    for (i=0; i<(int)frmline->word_count-1; i++)
                    {
                        if (frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER)
                        {
                            delta += expand_dx;
                            if (expand_dd>0)
                            {
                                delta++;
                                expand_dd--;
                            }
                        }
                        if ( i<(int)frmline->word_count-1 ) {
                            frmline->words[i+1].x += delta;
                        }
                    }
                    frmline->width += delta;
                }
            }

            // search for next uncommitted source object
            // last committed word
            formatted_word_t * lastword = &frmline->words[frmline->word_count-1];
            int lastSrcIndex = lastword->src_text_index; // last word index
            int nextSrcIndex = lastSrcIndex + 1;
            int nextSrcPos = 0;
            if ( !(lastword->flags & LTEXT_WORD_IS_OBJECT) ) {
                src_text_fragment_t * lastline = &m_pbuffer->srctext[lastSrcIndex];
                if ( lastword->t.start + lastword->t.len < lastline->t.offset + lastline->t.len ) {
                    // check whether last src line is finished
                    nextSrcPos = lastword->t.start + lastword->t.len;
                    nextSrcIndex = lastSrcIndex;
                }
            }
            setSrcLine( nextSrcIndex, nextSrcPos );

            if ( createNewLine ) {
                newline->y = frmline->y + frmline->height;
            }
            // go to new line
            frmline = newline;
        }
    }

    void setParagraphProperties( int index )
    {
        first_para_line = &m_pbuffer->srctext[index];
        align = (lUInt8)(first_para_line->flags & LTEXT_FLAG_NEWLINE);
        if ( !align )
            align = LTEXT_ALIGN_LEFT;
        interval = first_para_line->interval;
        //
        if ( frmline->word_count == 0 ) {
            // set margin
            frmline->x = first_para_line->margin;
        }
    }

    /**
     * Move to specified source line.
     * @param index is index of source line to go
     * @param pos is character position inside source line to go
     * 
     */
    void setSrcLine( int index, int pos )
    {
        if ( index >= (int)m_pbuffer->srctextlen ) {
            // last source line passed, no more lines
            srcline = NULL;
            return;
        }
        // leave on the same line
        if ( srcIndex == index ) {
            text_offset = pos;
            isLinkStart = ((line_flags & LTEXT_IS_LINK) != 0) && (pos==0); // first word of link
            return;
        } if ( index < srcIndex ) {
            // handle backward direction: update last paragraph properties
            int i;
            for ( i=index; i>0; i-- )
                if ( m_pbuffer->srctext[i].flags & LTEXT_FLAG_NEWLINE )
                    break;
            // set first_para_line, align, interval, margin, etc
            setParagraphProperties( i );
        }

        // update current line variables
        srcIndex = index;
        text_offset = pos;
        srcline = &m_pbuffer->srctext[srcIndex];
        line_flags = srcline->flags;
        isLinkStart = ((line_flags & LTEXT_IS_LINK) != 0) && (pos==0); // first word of link
        flgObject = (line_flags & LTEXT_SRC_IS_OBJECT) != 0; // object (e.g. image)
        if ( !flgObject )
        {
            font = (LVFont*)srcline->t.font;
        }

#if 0
        if ( !flgObject ) {
            lString16 srctext( srcline->t.text, srcline->t.len);
            lString8 srctext8 = UnicodeToUtf8(srctext);
            const char * srctxt = srctext8.c_str();
            if ( srctext8 == "Testing " ) {
                srctxt += 0;
            }
        }
#endif

        if ( srcIndex==0 && !(line_flags & LTEXT_FLAG_NEWLINE) )
            line_flags |= LTEXT_ALIGN_LEFT; /* first paragraph -- left by default */
        // update new paragraph properties
        if ( srcIndex==0 || (line_flags & LTEXT_FLAG_NEWLINE) ) {
            setParagraphProperties( srcIndex );
        }
        // check buffers size
        if (!flgObject && (int)widths_buf_size < (int)srcline->t.len + 64) //
        {
            widths_buf_size = srcline->t.len + 256;
            widths_buf.realloc( widths_buf_size );
            flags_buf.realloc( widths_buf_size );
        }

        vertical_align = srcline->flags & LTEXT_VALIGN_MASK;
        wy = 0;
        if ( !flgObject && vertical_align )
        {
            int fh = font->getHeight();
            if ( vertical_align == LTEXT_VALIGN_SUB )
            {
                wy += fh / 2;
            }
            else if ( vertical_align == LTEXT_VALIGN_SUPER )
            {
                wy -= fh / 2;
            }
        }
        flgLastParaLine = false;
        if (srcIndex==(int)m_pbuffer->srctextlen-1 || m_pbuffer->srctext[srcIndex+1].flags & LTEXT_FLAG_NEWLINE)
            flgLastParaLine = true;
        flgCanBreakBeforeNextLine = flgLastParaLine;
        if (!flgCanBreakBeforeNextLine) {
            src_text_fragment_t * nextline = &m_pbuffer->srctext[srcIndex + 1];
            if ( nextline->flags & LTEXT_SRC_IS_OBJECT )
                flgCanBreakBeforeNextLine = true;
            else {
                lChar16 firstChar = nextline->t.text[0];
                if ( lGetCharProps(firstChar) & CH_PROP_SPACE )
                    flgCanBreakBeforeNextLine = true;
            }
        }
    }

    /**
     * Add new word to formatted line.
     * @param firstch is index of first measured character of word
     * @param lastch is index of last measured character of word
     * @returns added word pointer
     */
    formatted_word_t * addWord( int firstch, int lastch )
    {
        formatted_word_t * word = lvtextAddFormattedWord( frmline );
        word->src_text_index = srcIndex;
        word->t.len = lastch - firstch + 1;
        int wpos = firstch>0 ? widths_buf[firstch-1] : 0;
        word->width = widths_buf[lastch] - wpos;
        if ( firstch>0 && (flags_buf[firstch-1] & LCHAR_ALLOW_HYPH_WRAP_AFTER) )
            word->width += font->getHyphenWidth();
        word->inline_width = word->width;
        word->t.start = text_offset + firstch;
        lUInt8 flags = 0;
        if ( isLinkStart ) {
            flags |= LTEXT_WORD_IS_LINK_START;
            isLinkStart = 0;
        }
        word->y = wy;

        // set word flags
        lChar16 lastchar = flags_buf[lastch];
        if (lastchar & LCHAR_ALLOW_WRAP_AFTER)
            flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
        if (lastchar & LCHAR_ALLOW_HYPH_WRAP_AFTER) {
            flags |= LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER;
            word->inline_width = word->width - font->getHyphenWidth();
        }
        if ( text_offset+lastch == srcline->t.len-1 && flgLastParaLine)
        {
            /* last char of src fragment */
            flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
        }
        if (lastchar & LCHAR_IS_SPACE) {
            flags |= LTEXT_WORD_CAN_ADD_SPACE_AFTER;
            if ( firstch<lastch )
                word->width = widths_buf[lastch-1] - wpos;
        }

        word->flags = flags;
        frmline->width += word->width;
        return word;
    }

    /**
     * Add new object (image) from current source to formatted line
     * @returns added object pointer
     */
    formatted_word_t * addObject()
    {
        formatted_word_t * word = lvtextAddFormattedWord( frmline );
        word->src_text_index = srcIndex;
        int scale_div = 1;
        int scale_mul = 1;
        int div_x = (srcline->o.width / m_pbuffer->width) + 1;
        int div_y = (srcline->o.height / m_pbuffer->page_height) + 1;
#if (MAX_IMAGE_SCALE_MUL==3)
        if ( srcline->o.height*3 < pbuffer->page_height-20
                && srcline->o.width*3 < pbuffer->width - 20 )
            scale_mul = 3;
        else
#endif
#if (MAX_IMAGE_SCALE_MUL==2) || (MAX_IMAGE_SCALE_MUL==3)
            if ( srcline->o.height*2 < pbuffer->page_height-20
                && srcline->o.width*2 < pbuffer->width - 20 )
            scale_mul = 2;
        else
#endif
        if (div_x>1 || div_y>1) {
            if (div_x>div_y)
                scale_div = div_x;
            else
                scale_div = div_y;
        }
        word->o.height = srcline->o.height * scale_mul / scale_div;
        word->width = srcline->o.width * scale_mul / scale_div;
        word->flags |= LTEXT_WORD_IS_OBJECT;
        word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
        word->y = 0;

        frmline->width += word->width;
        return word;
    }

    /**
     * @returns space left in current formatted line
     */
    inline int spaceLeft()
    {
        return m_pbuffer->width - (frmline->x + frmline->width);
    }

    /**
     * Format all data.
     */
    int format()
    {
        // start from first source line
        setSrcLine( 0, 0 );
        // until there is any source line left
        while ( srcline ) { //&& srcIndex < (int)m_pbuffer->srctextlen 
            if ( flgObject ) {
                // try to insert object
                addObject();
                int space_left = spaceLeft();
                if ( space_left<=0 )
                    commit( frmline->word_count-1 );
                else
                    setSrcLine( srcIndex+1, 0 );
            } else {
                // try to insert text
                int space_left = spaceLeft();
                int chars_left = srcline->t.len - text_offset;
                int chars_measured = font->measureText(
                        srcline->t.text + text_offset,
                        chars_left,
                        widths_buf.get(), flags_buf.get(),
                        space_left, //pbuffer->width,
                        '?');
                int j;
                int last_fit = -1;
                int last_hyph = -1;
                /* try to find good place for line break */
                if ( srcIndex>=(int)m_pbuffer->srctextlen || flgLastParaLine || flgCanBreakBeforeNextLine )
                        flags_buf.get()[chars_left-1] |= LCHAR_ALLOW_WRAP_AFTER;
                for (j = 0; j<chars_left && j<chars_measured; j++)
                {
                    if (widths_buf[j] > space_left)
                        break;
                    if (flags_buf[j] & LCHAR_ALLOW_WRAP_AFTER)
                        last_fit = j;
                    if (flags_buf[j] & LCHAR_ALLOW_HYPH_WRAP_AFTER)
                        last_hyph = j;
                }

                if ( last_hyph!=-1 ) {
                    if ( line_flags & LTEXT_HYPHENATE )
                        last_fit = last_hyph; // always hyphenate if enabled
                    else if ( last_fit==-1 && !frmline->word_count )
                        last_fit = last_hyph; // try to hyphenate if single word only
                }

                if ( last_fit==-1 ) {

                    int existingWrapPos = updateWrapPos( );
                    if ( existingWrapPos==0 ) {
                        //
                        /* try to find deprecated place for line break if good is not found */
                        for (j = 0; j<chars_left && j<chars_measured; j++)
                        {
                            if (widths_buf[j] > space_left)
                                break;
                            if (flags_buf[j] & LCHAR_DEPRECATED_WRAP_AFTER)
                                last_fit = j;
                        }
                        if (last_fit==-1) {
                            /* try to wrap in the middle of word */
                            for (j = 0; j<chars_left && j<chars_measured; j++)
                            {
                                if (widths_buf[j] > space_left)
                                    break;
                            }
                            if (j)
                                last_fit = j - 1;
                            else
                                last_fit = 0;
                        }
                    }
                }
                if ( last_fit==-1 ) {
                    // doesn't fit, commit already added words then try again from beginning of line
                    if ( chars_measured < chars_left )
                        commit();
                    else {
                        addWord(0, chars_measured-1);
                            //text_offset += chars_measured - (last_fit+1);
                        setSrcLine( srcIndex+1, 0 );
                    }
                } else {
                    // add words
                    if ( align == LTEXT_ALIGN_WIDTH ) {
                        //
                        int wstart, wpos;
                        for (j=0, wstart=0, wpos=0; j<=last_fit; j++)
                        {
                            if (flags_buf[j] & LCHAR_IS_SPACE || j==last_fit) /* LCHAR_ALLOW_WRAP_AFTER */
                            {
                                addWord( wstart, j );
                                wstart = j+1;
                            }
                        }
                    } else {
                        // add as single word
                        addWord( 0, last_fit );
                    }

                    // check rest of line
                    if ( last_fit<chars_left-1 ) {
                        // not all characters of source string fit into line
                        if ( chars_measured<chars_left ) {
                            // line is too wide
                            text_offset += last_fit + 1;
                            commit();
                        } else {
                            // line fits, but wrap is not allowed
                            addWord(last_fit + 1, chars_measured-1 );
                            setSrcLine( srcIndex+1, 0 );
                        }
                    } else if ( last_fit + 1 >= chars_left ) {
                        // all chars fit ok
                        if ( flgLastParaLine ) {
                            // if last paragraph line, commit
                            text_offset += last_fit + 1;
                            commit();
                        } else {
                            setSrcLine( srcIndex+1, 0 );
                        }
                    } else {
                        // not all chars fit
                        text_offset += last_fit + 1;
                    }
                }
                //
            }
        }
        commit();
        return frmline->y + frmline->height;
    }

    LVFormLine( formatted_text_fragment_t * buffer )
    : m_pbuffer(buffer)
    , widths_buf_size(1024)
    , widths_buf( widths_buf_size )
    , flags_buf( widths_buf_size )
    , srcIndex(-1)
    {
        newLine();
    }

    ~LVFormLine()
    {
    }
};

lUInt32 LFormattedText::FormatNew(lUInt16 width, lUInt16 page_height)
{ 
    // clear existing formatted data, if any
    if (m_pbuffer->frmlines)
    {
        for (lUInt32 i=0; i<m_pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( m_pbuffer->frmlines[i] );
        }
        free( m_pbuffer->frmlines );
    }
    m_pbuffer->frmlines = NULL;
    m_pbuffer->frmlinecount = 0;
    // setup new page size
    m_pbuffer->width = width;
    m_pbuffer->height = 0;
    m_pbuffer->page_height = page_height;
    // format text
    // text measurement buffer

    LVFormLine frmLine( m_pbuffer );

    // TODO: finish new implementation


    return frmLine.format();
}

void LFormattedText::Draw( LVDrawBuf * buf, int x, int y, ldomMarkedRangeList * marks )
{
    lUInt32 i, j;
    formatted_line_t * frmline;
    src_text_fragment_t * srcline;
    formatted_word_t * word;
    LVFont * font;
    lvRect clip;
    buf->GetClipRect( &clip );
    const lChar16 * str;
    int line_y = y;
    for (i=0; i<m_pbuffer->frmlinecount; i++)
    {
        if (line_y>=clip.bottom)
            break;
        frmline = m_pbuffer->frmlines[i];
        if (line_y + frmline->height>=clip.top)
        {
            // process background

            lUInt32 bgcl = buf->GetBackgroundColor();
            buf->FillRect( x+frmline->x, y + frmline->y, x+frmline->x + frmline->width, y + frmline->y + frmline->height, bgcl );

            // process marks
            if ( marks!=NULL && marks->length()>0 ) {
                lvRect lineRect( frmline->x, frmline->y, frmline->x + frmline->width, frmline->y + frmline->height );
                for ( int i=0; i<marks->length(); i++ ) {
                    lvRect mark;
                    ldomMarkedRange * range = marks->get(i);
                    if ( range->intersects( lineRect, mark ) ) {
                        //
                        buf->FillRect( mark.left + x, mark.top + y, mark.right + x, mark.bottom + y, 0xAAAAAA );
                    }
                }
            }
            for (j=0; j<frmline->word_count; j++)
            {
                word = &frmline->words[j];
                if (word->flags & LTEXT_WORD_IS_OBJECT)
                {
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    ldomElement * node = (ldomElement *) srcline->object;
                    LVImageSourceRef img = node->getObjectImageSource();
                    if ( img.isNull() )
                        img = LVCreateDummyImageSource( node, word->width, word->o.height );
                    int xx = x + frmline->x + word->x;
                    int yy = line_y + frmline->baseline - word->o.height + word->y;
                    buf->Draw( img, xx, yy, word->width, word->o.height );
                    //buf->FillRect( xx, yy, xx+word->width, yy+word->height, 1 );
                }
                else
                {
                    bool flgHyphen = false;
                    if ( (j==frmline->word_count-1) &&
                        (word->flags&LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER))
                        flgHyphen = true;
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    font = (LVFont *) srcline->t.font;
                    str = srcline->t.text + word->t.start;
                    /*
                    lUInt32 srcFlags = srcline->flags;
                    if ( srcFlags & LTEXT_BACKGROUND_MARK_FLAGS ) {
                        lvRect rc;
                        rc.left = x + frmline->x + word->x;
                        rc.top = line_y + (frmline->baseline - font->getBaseline()) + word->y;
                        rc.right = rc.left + word->width;
                        rc.bottom = rc.top + font->getHeight();
                        buf->FillRect( rc.left, rc.top, rc.right, rc.bottom, 0xAAAAAA );
                    }
                    */
                    lUInt32 oldColor = buf->GetTextColor();
                    lUInt32 oldBgColor = buf->GetBackgroundColor();
                    lUInt32 cl = srcline->color;
                    lUInt32 bgcl = srcline->bgcolor;
                    if ( cl!=0xFFFFFFFF )
                        buf->SetTextColor( cl );
                    if ( bgcl!=0xFFFFFFFF )
                        buf->SetBackgroundColor( bgcl );
                    font->DrawTextString(
                        buf,
                        x + frmline->x + word->x,
                        line_y + (frmline->baseline - font->getBaseline()) + word->y,
                        str,
                        word->t.len,
                        '?',
                        NULL,
                        flgHyphen,
                        srcline->flags & 0x0F00);
                    if ( cl!=0xFFFFFFFF )
                        buf->SetTextColor( oldColor );
                    if ( bgcl!=0xFFFFFFFF )
                        buf->SetBackgroundColor( oldBgColor );
                }
            }
        }
        line_y += frmline->height;
    }
}

#endif
