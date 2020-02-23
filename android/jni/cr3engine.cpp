// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>
#include <android/log.h>
#include <android/api-level.h>


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "org_coolreader_crengine_Engine.h"
#include "org_coolreader_crengine_DocView.h"

#include "cr3java.h"
#include "../../crengine/include/cr3version.h"
#include "docview.h"
#include "../../crengine/include/crengine.h"
#include "../../crengine/include/epubfmt.h"
#include "../../crengine/include/fb3fmt.h"
#include "../../crengine/include/pdbfmt.h"
#include "../../crengine/include/lvstream.h"


#include <../../crengine/include/fb2def.h>

#include "fc-lang-cat.h"

#define XS_IMPLEMENT_SCHEME 1
#include <../../crengine/include/fb2def.h>
#include <sys/stat.h>
#include <lvopc.h>

#if defined(__arm__) || defined(__aarch64__) || defined(__i386__) || defined(__mips__)
#define USE_COFFEECATCH 1
#endif


#if USE_COFFEECATCH == 1
#include "coffeecatch/coffeecatch.h"
#include "coffeecatch/coffeejni.h"
#else
#define COFFEE_TRY_JNI(ENV, CODE) CODE;
#endif

#ifdef _DEBUG
// missing in system ZLIB with DEBUG option turned off
int z_verbose=0;
extern "C" void z_error(char * msg);
void z_error(char * msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
#endif
/// returns current time representation string
static lString16 getDateTimeString( time_t t )
{
    tm * bt = localtime(&t);
    char str[32];
#ifdef _LINUX
    snprintf(str, 32,
#else
    sprintf(str, 
#endif
        "%04d/%02d/%02d %02d:%02d", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min);
    str[31] = 0;
    return Utf8ToUnicode( lString8( str ) );
}

#if 0
static lString16 extractDocSeriesReverse( ldomDocument * doc, int & seriesNumber )
{
	seriesNumber = 0;
    lString16 res;
    ldomXPointer p = doc->createXPointer(L"/FictionBook/description/title-info/sequence");
    if ( p.isNull() )
        return res;
    ldomNode * series = p.getNode();
    if ( series ) {
        lString16 sname = series->getAttributeValue( attr_name );
        lString16 snumber = series->getAttributeValue( attr_number );
        if ( !sname.empty() ) {
            res << L"(";
            if ( !snumber.empty() ) {
                res << L"#" << snumber << L" ";
                seriesNumber = snumber.atoi();
            }
            res << sname;
            res << L")";
        }
    }
    return res;
}

static lString16 extractDocPublSeriesReverse( ldomDocument * doc, int & seriesNumber )
{
	seriesNumber = 0;
    lString16 res;
    ldomXPointer p = doc->createXPointer(L"/FictionBook/description/publish-info/sequence");
    if ( p.isNull() )
        return res;
    ldomNode * series = p.getNode();
    if ( series ) {
        lString16 sname = series->getAttributeValue( attr_name );
        lString16 snumber = series->getAttributeValue( attr_number );
        if ( !sname.empty() ) {
            res << L"(";
            if ( !snumber.empty() ) {
                res << L"#" << snumber << L" ";
                seriesNumber = snumber.atoi();
            }
            res << sname;
            res << L")";
        }
    }
    return res;
}
#endif

class BookProperties
{
public:
    lString16 filename;
    lString16 title;
    lString16 author;
    lString16 series;
    int filesize;
    lString16 filedate;
    int seriesNumber;
    lString16 language;
	lString16 genre;
	lString16 annotation;
	lString16 srclang;
	lString16 bookdate;
    lString16 translator;
    lString16 docauthor;
    lString16 docprogram;
    lString16 docdate;
    lString16 docsrcurl;
    lString16 docsrcocr;
    lString16 docversion;
    lString16 publname;
    lString16 publisher;
    lString16 publcity;
    lString16 publyear;
    lString16 publisbn;
	lString16 publseries;
	int publseriesNumber;
};

static bool GetEPUBBookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
    LVContainerRef m_arc = LVOpenArchieve( stream );
    if ( m_arc.isNull() )
        return false; // not a ZIP` archive

    // check root media type
    lString16 rootfilePath = EpubGetRootFilePath(m_arc);
    if ( rootfilePath.empty() )
    	return false;

    lString16 codeBase;
    codeBase=LVExtractPath(rootfilePath, false);

    LVStreamRef content_stream = m_arc->OpenStream(rootfilePath.c_str(), LVOM_READ);
    if ( content_stream.isNull() )
        return false;

    ldomDocument * doc = LVParseXMLStream( content_stream );
    if ( !doc )
        return false;

    time_t t = (time_t)time(0);
    struct stat fs;
    if ( !stat( name, &fs ) ) {
        t = fs.st_mtime;
    }

    lString16 author = doc->textFromXPath( lString16("package/metadata/creator")).trim();
    lString16 title = doc->textFromXPath( lString16("package/metadata/title")).trim();
    lString16 language = doc->textFromXPath( lString16("package/metadata/language")).trim();
	lString16 bookdate = doc->textFromXPath( lString16("package/metadata/date")).trim();
	lString16 annotation = doc->textFromXPath( lString16("package/metadata/description")).trim();
	lString16 publisher = doc->textFromXPath( lString16("package/metadata/publisher")).trim();
	lString16 genre = doc->textFromXPath( lString16("package/metadata/subject")).trim();
	lString16 docsrcurl = doc->textFromXPath( lString16("package/metadata/identifier")).trim();
	lString16 docsrcocr = doc->textFromXPath( lString16("package/metadata/source")).trim();

    pBookProps->author = author;
    pBookProps->title = title;
    pBookProps->language = language;
	pBookProps->bookdate = bookdate;
	pBookProps->annotation = annotation;
 	pBookProps->publisher = publisher;
 	pBookProps->genre = genre;
 	pBookProps->docsrcurl = docsrcurl;
	pBookProps->docsrcocr = docsrcocr;

    for ( int i=1; i<20; i++ ) {
        ldomNode * item = doc->nodeFromXPath( lString16("package/metadata/meta[") << fmt::decimal(i) << "]" );
        if ( !item )
            break;
        lString16 name = item->getAttributeValue("name");
        lString16 content = item->getAttributeValue("content");
        if (name == "calibre:series")
        	pBookProps->series = content.trim();
        else if (name == "calibre:series_index")
        	pBookProps->seriesNumber = content.trim().atoi();
    }

    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    pBookProps->filedate = getDateTimeString( t );

    delete doc;

    return true;
}

static bool GetFB3BookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
	ldomDocument * descDoc = NULL;
	descDoc = Fb3GetDescDoc( stream );

	if ( !descDoc )
		return false;

	lString16 title = descDoc->textFromXPath( lString16("title/main"));
	lString16 authors;
	lString16 annotation = descDoc->textFromXPath( lString16("annotation"));
	lString16 language = descDoc->textFromXPath( lString16("lang"));
	lString16 translators;
	lString16 publishers;
	lString16 genres;
	lString16 srclang = descDoc->textFromXPath( lString16("written/lang"));
	lString16 bookdate = descDoc->textFromXPath( lString16("written/date"));

	for ( int i=1; i<30; i++ ) {
		ldomNode * item = descDoc->nodeFromXPath(lString16("fb3-relations/subject[") << fmt::decimal(i) << "]");
		if ( !item ) break;
		lString16 name = item->getAttributeValue("link");
		if (name == "author") {
			lString16 author = descDoc->textFromXPath( lString16("fb3-relations/subject[") << fmt::decimal(i) << "]/title/main");
			if ( !authors.empty() ) {
				authors += " |";
			}
			authors += author;
		}
		if (name == "translator") {
			lString16 translator = descDoc->textFromXPath( lString16("fb3-relations/subject[") << fmt::decimal(i) << "]/title/main");
			if ( !translators.empty() ) {
				translators += " | ";
			}
			translators += translator;
		}
		if (name == "publisher") {
			lString16 publisher = descDoc->textFromXPath( lString16("fb3-relations/subject[") << fmt::decimal(i) << "]/title/main");
			if ( !publishers.empty() ) {
				publishers += " | ";
			}
			publishers += publisher;
		}
	}
	for ( int i=1; i<30; i++ ) {
		ldomNode * item = descDoc->nodeFromXPath(lString16("fb3-classification/subject[") << fmt::decimal(i) << "]");
		if ( !item ) break;
		lString16 genre = descDoc->textFromXPath( lString16("fb3-classification/subject[") << fmt::decimal(i) << "]");
		if ( !genres.empty() ) {
			genres += " | ";
		}
		genres += genre;
	}

	ldomNode * item = descDoc->nodeFromXPath(lString16("document-info"));
    lString16 created = cs16("");
    lString16 docprogram = cs16("");
    lString16 docauthor = cs16("");
    lString16 docsrcurl = cs16("");
    lString16 docsrcocr = cs16("");
	if (item) {
        created = item->getAttributeValue("created");
        docprogram = item->getAttributeValue("program-used");
        docauthor = item->getAttributeValue("editor");
        docsrcurl = item->getAttributeValue("src-url");
        docsrcocr = item->getAttributeValue("ocr");
    }
	lString16 series = descDoc->textFromXPath(lString16("fb3-description/sequence/title/main"));
	ldomNode * item2 = descDoc->nodeFromXPath(lString16("paper-publish-info"));
    lString16 publisher = cs16("");
    lString16 city = cs16("");
    lString16 year = cs16("");
    if ( item2 ) {
        publisher = item2->getAttributeValue("publisher");
        city = item2->getAttributeValue("city");
        year = item2->getAttributeValue("year");
    }
	lString16 publisbn = descDoc->textFromXPath(lString16("paper-publish-info/isbn"));
	lString16 publsequence = descDoc->textFromXPath(lString16("paper-publish-info/sequence"));

	pBookProps-> title = title;
	pBookProps-> author = authors;
	pBookProps-> annotation = annotation;
	pBookProps-> language = language;
	pBookProps-> translator = translators;
	pBookProps-> publisher = publishers;
	pBookProps-> genre = genres;
	pBookProps-> srclang = srclang;
	pBookProps-> bookdate = bookdate;
	pBookProps-> docdate = created;
	pBookProps->docprogram = docprogram;
 	pBookProps->series = series;
	pBookProps->publisher = publisher;
	pBookProps->publcity = city;
	pBookProps->publyear = year;
	pBookProps->publisbn = publisbn;
	pBookProps->publseries = publsequence;
	pBookProps->docauthor = docauthor;
	pBookProps->docsrcurl = docsrcurl;
	pBookProps->docsrcocr = docsrcocr;

	time_t t = (time_t)time(0);
	struct stat fs;
	if ( !stat( name, &fs ) ) {
		t = fs.st_mtime;
	}

	pBookProps->filesize = (long)stream->GetSize();
	pBookProps->filename = lString16(name);
	pBookProps->filedate = getDateTimeString( t );

	delete descDoc;

	return true;
}

static bool GetBookProperties(const char *name,  BookProperties * pBookProps)
{
    CRLog::trace("GetBookProperties( %s )", name);

    // check archieve
    lString16 arcPathName;
    lString16 arcItemPathName;
    bool isArchiveFile = LVSplitArcName( lString16(name), arcPathName, arcItemPathName );

    // open stream
    LVStreamRef stream = LVOpenFileStream( (isArchiveFile ? arcPathName : Utf8ToUnicode(lString8(name))).c_str() , LVOM_READ);
    if (!stream) {
        CRLog::error("cannot open file %s", name);
        return false;
    }


    if ( DetectEpubFormat( stream ) ) {
        CRLog::trace("GetBookProperties() : epub format detected");
    	return GetEPUBBookProperties( name, stream, pBookProps );
    }

	if ( DetectFb3Format( stream ) ) {
		CRLog::trace("GetBookProperties() : fb3 format detected");
		return GetFB3BookProperties( name, stream, pBookProps );
	}

    time_t t = (time_t)time(0);

    if ( isArchiveFile ) {
        int arcsize = (int)stream->GetSize();
        LVContainerRef container = LVOpenArchieve(stream);
        if ( container.isNull() ) {
            CRLog::error( "Cannot read archive contents from %s", LCSTR(arcPathName) );
            return false;
        }
        stream = container->OpenStream(arcItemPathName.c_str(), LVOM_READ);
        if ( stream.isNull() ) {
            CRLog::error( "Cannot open archive file item stream %s", LCSTR(lString16(name)) );
            return false;
        }
    }
    struct stat fs;
    if ( !stat( name, &fs ) ) {
        t = fs.st_mtime;
    }

    // read document
#if COMPACT_DOM==1
    ldomDocument doc(stream, 0);
#else
    ldomDocument doc;
#endif
    ldomDocumentWriter writer(&doc, true);
    doc.setNodeTypes( fb2_elem_table );
    doc.setAttributeTypes( fb2_attr_table );
    doc.setNameSpaceTypes( fb2_ns_table );
    LVXMLParser parser( stream, &writer );
    CRLog::trace( "checking format..." );
	if ( !parser.CheckFormat() ) {
        return false;
    }
    CRLog::trace( "parsing..." );
    if ( !parser.Parse() ) {
        return false;
    }
    CRLog::trace( "parsed" );
    #if 0
        char ofname[512];
        sprintf(ofname, "%s.xml", name);
        CRLog::trace("    writing to file %s", ofname);
        LVStreamRef out = LVOpenFileStream(ofname, LVOM_WRITE);
        doc.saveToStream(out, "utf16");
    #endif
    lString16 authors = extractDocAuthors( &doc, lString16("|"), false );
    lString16 title = extractDocTitle( &doc );
    lString16 language = extractDocLanguage( &doc );
    lString16 series = extractDocSeries( &doc, &pBookProps->seriesNumber );
	lString16 genre = extractDocGenre( &doc, lString16("|") );
	lString16 annotation = extractDocAnnotation( &doc );
	lString16 srclang = extractDocSrcLang( &doc );
	lString16 bookdate = extractDocBookDate( &doc );
	lString16 translator = extractDocTranslator( &doc, false );
    lString16 docauthor = extractDocAuthor( &doc );
    lString16 docprogram = extractDocProgram( &doc );
    lString16 docdate = extractDocDate( &doc );
    lString16 docsrcurl = extractDocSrcUrl( &doc );
    lString16 docsrcocr = extractDocSrcOcr( &doc );
    lString16 docversion = extractDocVersion( &doc );
    lString16 publname = extractPublName( &doc );
    lString16 publisher = extractPublisher( &doc );
    lString16 publcity = extractPublCity( &doc );
    lString16 publyear = extractPublYear( &doc );
    lString16 publisbn = extractPublIsbn( &doc );
	lString16 publseries = extractDocPublishSeries( &doc, &pBookProps->publseriesNumber );

#if SERIES_IN_AUTHORS==1
    if ( !series.empty() )
        authors << "    " << series;
#endif
    pBookProps->title = title;
    pBookProps->author = authors;
    pBookProps->series = series;
    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    pBookProps->filedate = getDateTimeString( t );
    pBookProps->language = language;
	pBookProps->genre = genre;
	pBookProps->annotation = annotation;
	pBookProps->srclang = srclang;
	pBookProps->bookdate = bookdate;
	pBookProps->translator = translator;
    pBookProps->docauthor = docauthor;
    pBookProps->docprogram = docprogram;
    pBookProps->docdate = docdate;
    pBookProps->docsrcurl = docsrcurl;
    pBookProps->docsrcocr = docsrcocr;
    pBookProps->docversion = docversion;
    pBookProps->publname = publname;
    pBookProps->publisher = publisher;
    pBookProps->publcity = publcity;
    pBookProps->publyear = publyear;
    pBookProps->publisbn = publisbn;
	pBookProps->publseries = publseries;
	return true;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    scanBookPropertiesInternal
 * Signature: (Lorg/coolreader/crengine/FileInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal
  (JNIEnv * _env, jclass _engine, jobject _fileInfo)
{
	CRJNIEnv env(_env);
	jclass objclass = env->GetObjectClass(_fileInfo);
	jfieldID fid = env->GetFieldID(objclass, "pathname", "Ljava/lang/String;");
	lString16 filename = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
    fid = env->GetFieldID(objclass, "arcname", "Ljava/lang/String;");
    lString16 arcname = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
	if ( filename.empty() )
		return JNI_FALSE;
	if ( !arcname.empty() )
       filename = arcname + "@/" + filename;

	BookProperties props;
	CRLog::debug("Looking for properties of file %s", LCSTR(filename));
	bool res = GetBookProperties(LCSTR(filename),  &props);
	if ( !res )
		return JNI_FALSE;
	#define SET_STR_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "Ljava/lang/String;"); \
	    env->SetObjectField(_fileInfo,fid,env.toJavaString(src)); \
	}
	#define SET_INT_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "I"); \
	    env->SetIntField(_fileInfo,fid,src); \
	}
	SET_STR_FLD("title",props.title);
	SET_STR_FLD("authors",props.author);
	SET_STR_FLD("series",props.series);
	SET_INT_FLD("seriesNumber",props.seriesNumber);
	SET_STR_FLD("language",props.language);
	SET_STR_FLD("genre",props.genre);
	SET_STR_FLD("annotation",props.annotation);
	SET_STR_FLD("srclang",props.srclang);
	SET_STR_FLD("bookdate",props.bookdate);
	SET_STR_FLD("translator",props.translator);
    SET_STR_FLD("docauthor",props.docauthor);
    SET_STR_FLD("docprogram",props.docprogram);
    SET_STR_FLD("docdate",props.docdate);
    SET_STR_FLD("docsrcurl",props.docsrcurl);
    SET_STR_FLD("docsrcocr",props.docsrcocr);
    SET_STR_FLD("docversion",props.docversion);
    SET_STR_FLD("publname",props.publname);
    SET_STR_FLD("publisher",props.publisher);
    SET_STR_FLD("publcity",props.publcity);
    SET_STR_FLD("publyear",props.publyear);
    SET_STR_FLD("publisbn",props.publisbn);
	SET_STR_FLD("publseries",props.publseries);
	SET_INT_FLD("publseriesNumber",props.publseriesNumber);

	return JNI_TRUE;
}


void drawBookCoverInternal(JNIEnv * _env, jclass _engine, jobject bitmap, jbyteArray _data, jstring _fontFace, jstring _title, jstring _authors, jstring _seriesName, jint seriesNumber, jint bpp)
{
	CRJNIEnv env(_env);
	CRLog::debug("drawBookCoverInternal called");
	lString8 fontFace = UnicodeToUtf8(env.fromJavaString(_fontFace));
	lString16 title = env.fromJavaString(_title);
	lString16 authors = env.fromJavaString(_authors);
	lString16 seriesName = env.fromJavaString(_seriesName);
	LVStreamRef stream;
	LVDrawBuf * drawbuf = BitmapAccessorInterface::getInstance()->lock(_env, bitmap);
	if (drawbuf != NULL) {
		LVImageSourceRef image;
		if (_data != NULL && _env->GetArrayLength(_data) > 0) {
			CRLog::debug("drawBookCoverInternal : cover image from array");
			stream = env.jbyteArrayToStream(_data);
			if (!stream.isNull())
				image = LVCreateStreamImageSource(stream);
		}

		int factor = 1;
		int dx = drawbuf->GetWidth();
		int dy = drawbuf->GetHeight();
		int MIN_WIDTH = 300;
		int MIN_HEIGHT = 400;
		if (dx < MIN_WIDTH || dy < MIN_HEIGHT) {
			if (dx * 2 < MIN_WIDTH || dy * 2 < MIN_HEIGHT) {
				dx *= 3;
				dy *= 3;
				factor = 3;
			} else {
				dx *= 2;
				dy *= 2;
				factor = 2;
			}
		}
		LVDrawBuf * drawbuf2 = drawbuf;
		if (factor > 1)
			drawbuf2 = new LVColorDrawBuf(dx, dy, drawbuf->GetBitsPerPixel());

		if (bpp >= 16) {
			// native color resolution
			CRLog::debug("drawBookCoverInternal : calling LVDrawBookCover");
			LVDrawBookCover(*drawbuf2, image, fontFace, title, authors, seriesName, seriesNumber);
			image.Clear();
		} else {
			LVGrayDrawBuf grayBuf(drawbuf2->GetWidth(), drawbuf2->GetHeight(), bpp);
			LVDrawBookCover(grayBuf, image, fontFace, title, authors, seriesName, seriesNumber);
			image.Clear();
			grayBuf.DrawTo(drawbuf2, 0, 0, 0, NULL);
		}

		if (factor > 1) {
			CRLog::debug("drawBookCoverInternal : rescaling");
			drawbuf->DrawRescaled(drawbuf2, 0, 0, drawbuf->GetWidth(), drawbuf->GetHeight(), 0);
			delete drawbuf2;
		}

		//CRLog::trace("getPageImageInternal calling bitmap->unlock");
		BitmapAccessorInterface::getInstance()->unlock(_env, bitmap, drawbuf);
	} else {
		CRLog::error("bitmap accessor is invalid");
	}
	CRLog::debug("drawBookCoverInternal finished");
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    drawBookCoverInternal
 * Signature: (Landroid/graphics/Bitmap;[BLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_drawBookCoverInternal
  (JNIEnv * _env, jclass _engine, jobject bitmap, jbyteArray _data, jstring _fontFace, jstring _title, jstring _authors, jstring _seriesName, jint seriesNumber, jint bpp)
{
	COFFEE_TRY_JNI(_env, drawBookCoverInternal(_env, _engine, bitmap, _data, _fontFace, _title, _authors, _seriesName, seriesNumber, bpp));
}

jbyteArray scanBookCoverInternal
  (JNIEnv * _env, jclass _class, jstring _path)
{
	CRJNIEnv env(_env);
	lString16 path = env.fromJavaString(_path);
	CRLog::debug("scanBookCoverInternal(%s) called", LCSTR(path));
	lString16 arcname, item;
    LVStreamRef res;
    jbyteArray array = NULL;
    LVContainerRef arc;
	if (!LVSplitArcName(path, arcname, item)) {
		// not in archive
		LVStreamRef stream = LVOpenFileStream(path.c_str(), LVOM_READ);
		if (!stream.isNull()) {
			arc = LVOpenArchieve(stream);
			if (!arc.isNull()) {
				// ZIP-based format
				if (DetectEpubFormat(stream)) {
					// EPUB
					// extract coverpage from epub
					res = GetEpubCoverpage(arc);
				}
				if (DetectFb3Format(stream)) {
					// FB3
					// extract coverpage from FB3
					res = GetFb3Coverpage(arc);
				}
			} else {
				res = GetFB2Coverpage(stream);
				if (res.isNull()) {
					doc_format_t fmt;
					if (DetectPDBFormat(stream, fmt)) {
						res = GetPDBCoverpage(stream);
					}
				}
			}
		}
	} else {
    	CRLog::debug("scanBookCoverInternal() : is archive, item=%s, arc=%s", LCSTR(item), LCSTR(arcname));
		LVStreamRef arcstream = LVOpenFileStream(arcname.c_str(), LVOM_READ);
		if (!arcstream.isNull()) {
			arc = LVOpenArchieve(arcstream);
			if (!arc.isNull()) {
				LVStreamRef stream = arc->OpenStream(item.c_str(), LVOM_READ);
				if (!stream.isNull()) {
			    	CRLog::debug("scanBookCoverInternal() : archive stream opened ok, parsing");
					res = GetFB2Coverpage(stream);
					if (res.isNull()) {
						doc_format_t fmt;
						if (DetectPDBFormat(stream, fmt)) {
							res = GetPDBCoverpage(stream);
						}
					}
				}
			}
		}
	}
	if (!res.isNull())
		array = env.streamToJByteArray(res);
    if (array != NULL)
    	CRLog::debug("scanBookCoverInternal() : returned cover page array");
    else
    	CRLog::debug("scanBookCoverInternal() : cover page data not found");
    return array;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    scanBookCoverInternal
 * Signature: (Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_coolreader_crengine_Engine_scanBookCoverInternal
  (JNIEnv * _env, jclass _class, jstring _path)
{
	jbyteArray res = NULL;
	COFFEE_TRY_JNI(_env, res = scanBookCoverInternal( _env, _class, _path));
	return res;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getArchiveItemsInternal
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getArchiveItemsInternal
  (JNIEnv * _env, jclass, jstring jarcName)
{
    CRJNIEnv env(_env);
    lString16 arcName = env.fromJavaString(jarcName);
    lString16Collection list;
    
    //fontMan->getFaceList(list);
    LVStreamRef stream = LVOpenFileStream( arcName.c_str(), LVOM_READ );
    if ( !stream.isNull() ) {
        LVContainerRef arc = LVOpenArchieve(stream);
        if ( !arc.isNull() ) {
            // convert
            for ( int i=0; i<arc->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = arc->GetObjectInfo(i);
                if ( item->IsContainer())
                    continue;
                list.add( item->GetName() );
                list.add( lString16::itoa(item->GetSize()) );
            }
        }
    }
    return env.toJavaStringArray(list);
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setHyphenationMethod
 * Signature: (I[B)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setHyphenationMethod
  (JNIEnv * _env, jobject _engine, jint method, jbyteArray data)
{
	CRJNIEnv env(_env);
	if ( method==0 ) {
		CRLog::info("Selecting hyphenation method: Disabled");
		return HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
	} else if ( method==1 ) {
		CRLog::info("Selecting hyphenation method: Algoryphmic");
		return HyphMan::activateDictionary(lString16(HYPH_DICT_ID_ALGORITHM));
	} else {
		CRLog::info("Selecting hyphenation method: Dictionary");
		LVStreamRef stream = env.jbyteArrayToStream( data );
		CRLog::debug("Stream is created from byte array, length=%d", (int)(stream.isNull()?0:stream->GetSize()));
		bool res = HyphMan::activateDictionaryFromStream(stream);
		if ( !res ) {
			CRLog::error("Dictionary activation is failed: disabling hyphenation");
			HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
			return false;
		}
		return true;
	}
}


class JNICDRLogger : public CRLog
{
public:
    JNICDRLogger()
    {
    	curr_level = CRLog::LL_DEBUG;
    }
protected:
  
	virtual void log( const char * lvl, const char * msg, va_list args)
	{
	    #define MAX_LOG_MSG_SIZE 1024
		static char buffer[MAX_LOG_MSG_SIZE+1];
		vsnprintf(buffer, MAX_LOG_MSG_SIZE, msg, args);
		int level = ANDROID_LOG_DEBUG;
		//LOGD("CRLog::log is called with LEVEL %s, pattern %s", lvl, msg);
		if ( !strcmp(lvl, "FATAL") )
			level = ANDROID_LOG_FATAL;
		else if ( !strcmp(lvl, "ERROR") )
			level = ANDROID_LOG_ERROR;
		else if ( !strcmp(lvl, "WARN") )
			level = ANDROID_LOG_WARN;
		else if ( !strcmp(lvl, "INFO") )
			level = ANDROID_LOG_INFO;
		else if ( !strcmp(lvl, "DEBUG") )
			level = ANDROID_LOG_DEBUG;
		else if ( !strcmp(lvl, "TRACE") )
			level = ANDROID_LOG_VERBOSE;
		__android_log_write(level, LOG_TAG, buffer);
	}
};

//typedef void (lv_FatalErrorHandler_t)(int errorCode, const char * errorText );

void cr3androidFatalErrorHandler(int errorCode, const char * errorText )
{
	LOGE("CoolReader Fatal Error #%d: %s", errorCode, errorText);
	LOGASSERTFAILED("CoolReader Fatal Error", "CoolReader Fatal Error #%d: %s", errorCode, errorText);
	//static char str[1001];
	//snprintf(str, 1000, "CoolReader Fatal Error #%d: %s", errorCode, errorText);
	//LOGE("CoolReader Fatal Error #%d: %s", errorCode, errorText);
	//LOGASSERTFAILED(errorText, "CoolReader Fatal Error #%d: %s", errorCode, errorText);
}

/// set fatal error handler
void crSetFatalErrorHandler( lv_FatalErrorHandler_t * handler );

jboolean initInternal(JNIEnv * penv, jclass obj, jobjectArray fontArray, jint sdk_int) {

	CRJNIEnv::sdk_int = sdk_int;

	CRJNIEnv env(penv);

	// to catch crashes and remove current cache file on crash (SIGSEGV etc.)
	crSetSignalHandler();

	LOGI("initInternal called");
	// set fatal error handler
	crSetFatalErrorHandler( &cr3androidFatalErrorHandler );
	LOGD("Redirecting CDRLog to Android");
	CRLog::setLogger( new JNICDRLogger() );
	CRLog::setLogLevel( CRLog::LL_TRACE );
	CRLog::info("CREngine log redirected");
	CRLog::info("CRENGINE version %s %s", CR_ENGINE_VERSION, CR_ENGINE_BUILD_DATE);
	
	CRLog::info("initializing hyphenation manager");
    HyphMan::initDictionaries(lString16::empty_str); //don't look for dictionaries
	HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
	CRLog::info("creating font manager");
    InitFontManager(lString8::empty_str);
	CRLog::debug("converting fonts array: %d items", (int)env->GetArrayLength(fontArray));
	lString16Collection fonts;
	env.fromJavaStringArray(fontArray, fonts);
	int len = fonts.length();
	CRLog::debug("registering fonts: %d fonts in list", len);
	for ( int i=0; i<len; i++ ) {
		lString8 fontName = UnicodeToUtf8(fonts[i]);
		CRLog::debug("registering font %s", fontName.c_str());
		if ( !fontMan->RegisterFont( fontName ) )
			CRLog::error("cannot load font %s", fontName.c_str());
	}
    CRLog::info("%d fonts registered", fontMan->GetFontCount());
	return fontMan->GetFontCount() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    initInternal
 * Signature: ([Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_initInternal
  (JNIEnv * penv, jclass obj, jobjectArray fontArray, jint sdk_int)
{
	jboolean res = JNI_FALSE;
	COFFEE_TRY_JNI(penv, res = initInternal(penv, obj, fontArray, sdk_int));
	return res;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    uninitInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_uninitInternal
  (JNIEnv *, jclass)
{
	LOGI("uninitInternal called");
	HyphMan::uninit();
	ShutdownFontManager();
	CRLog::setLogger(NULL);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getFontFaceListInternal
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getFontFaceListInternal
  (JNIEnv * penv, jclass obj)
{
	LOGI("getFontFaceListInternal called");
	CRJNIEnv env(penv);
	lString16Collection list;
	COFFEE_TRY_JNI(penv, fontMan->getFaceList(list));
	return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getFontFileNameListInternal
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getFontFileNameListInternal
        (JNIEnv * penv, jclass cls)
{
    LOGI("getFontFileListInternal called");
    CRJNIEnv env(penv);
    lString16Collection list;
    COFFEE_TRY_JNI(penv, fontMan->getFontFileNameList(list));
    return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getFontFaceAndFileNameList
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getFontFaceAndFileNameListInternal
		(JNIEnv * penv, jclass cls)
{
	LOGI("getFontFileListInternal called");
	CRJNIEnv env(penv);
	lString16Collection list;
	COFFEE_TRY_JNI(penv, fontMan->getFontFaceAndFileNameList(list));
	return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setCacheDirectoryInternal
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal
  (JNIEnv * penv, jclass obj, jstring dir, jint size)
{
	CRJNIEnv env(penv);
	bool res = false;
	COFFEE_TRY_JNI(penv, res = ldomDocCache::init(env.fromJavaString(dir), size ));
	return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    haveFcLangCodeInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_haveFcLangCodeInternal
		(JNIEnv *env, jclass cls, jstring langCode)
{
	jboolean res = JNI_FALSE;
	const char* langCode_ptr = env->GetStringUTFChars(langCode, 0);
	if (langCode_ptr) {
		struct fc_lang_catalog* lang_ptr = fc_lang_cat;
		for (int i = 0; i < fc_lang_cat_sz; i++)
		{
			if (strcmp(lang_ptr->lang_code, langCode_ptr) == 0)
			{
				res = JNI_TRUE;
				break;
			}
			lang_ptr++;
		}
		env->ReleaseStringUTFChars(langCode, langCode_ptr);
	}
	return res;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    checkFontLanguageCompatibilityInternal
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_checkFontLanguageCompatibilityInternal
		(JNIEnv *env, jclass cls, jstring fontFace, jstring langCode)
{
	jboolean res = JNI_TRUE;
	const char* fontFace_ptr = env->GetStringUTFChars(fontFace, 0);
	const char* langCode_ptr = env->GetStringUTFChars(langCode, 0);
	if (fontFace_ptr && langCode_ptr) {
		res = fontMan->checkFontLangCompat(lString8(fontFace_ptr), lString8(langCode_ptr)) ? JNI_TRUE : JNI_FALSE;
	}
	if (langCode_ptr)
		env->ReleaseStringUTFChars(langCode, langCode_ptr);
	if (fontFace_ptr)
		env->ReleaseStringUTFChars(fontFace, fontFace_ptr);
	return res;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    listFilesInternal
 * Signature: (Ljava/io/File;)[Ljava/io/File;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_listFilesInternal
		(JNIEnv *penv, jclass, jobject jdir)
{
	CRJNIEnv env(penv);
	if (NULL == jdir)
		return NULL;
	jclass pjcFile = env->FindClass("java/io/File");
	if (NULL == pjcFile)
		return NULL;
	jmethodID pjmFile_GetAbsolutePath = env->GetMethodID(pjcFile, "getAbsolutePath", "()Ljava/lang/String;");
	if (NULL == pjmFile_GetAbsolutePath)
		return NULL;
	jmethodID pjmFile_Ctor = env->GetMethodID(pjcFile, "<init>", "(Ljava/lang/String;)V");
	if (NULL == pjmFile_Ctor)
		return NULL;
	jstring jpathname = (jstring)env->CallObjectMethod(jdir, pjmFile_GetAbsolutePath);
	if (NULL == jpathname)
		return NULL;
	lString16 path = env.fromJavaString(jpathname);
	jobjectArray jarray = NULL;
	LVContainerRef dir = LVOpenDirectory(path);
	if ( !dir.isNull() ) {
		jstring emptyString = env->NewStringUTF("");
		jobject emptyFile = env->NewObject(pjcFile, pjmFile_Ctor, emptyString);
		jarray = env->NewObjectArray(dir->GetObjectCount(), pjcFile, emptyFile);
		if (NULL != jarray) {
			for (int i = 0; i < dir->GetObjectCount(); i++) {
				const LVContainerItemInfo *item = dir->GetObjectInfo(i);
				if (item && item->GetName()) {
					lString16 fileName = path + "/" + item->GetName();
					jstring jfilename = env.toJavaString(fileName);
					if (NULL != jfilename) {
						env->ExceptionClear();
						jobject jfile = env->NewObject(pjcFile, pjmFile_Ctor, jfilename);
						if (env->ExceptionCheck() == JNI_TRUE)
							env->ExceptionClear();
						else {
							if (NULL != jfile)
								env->SetObjectArrayElement(jarray, i, jfile);
						}
						env->DeleteLocalRef(jfile);
						env->DeleteLocalRef(jfilename);
					}
				}
			}
		}
	}
	return jarray;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    isLink
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_coolreader_crengine_Engine_isLink
  (JNIEnv * env, jclass obj, jstring pathname)
{
	//CRLog::trace("isLink : enter");
	if (!pathname)
		return NULL;
	//CRLog::trace("isLink : pathname is not null");
	int res = JNI_FALSE;
	jboolean iscopy;
	const char * s = env->GetStringUTFChars(pathname, &iscopy);
	//CRLog::trace("isLink : read utf from pathname");
	struct stat st;
	lString8 path;
	if ( !lstat( s, &st) ) {
		if ( S_ISLNK(st.st_mode) ) {
			char buf[2048];
			int len = readlink(s, buf, sizeof(buf) - 1);
			if (len != -1) {
				buf[len] = 0;
				path = lString8(buf);
			}
		}
	}
	//CRLog::trace("isLink : releasing utf pathname");
	env->ReleaseStringUTFChars(pathname, s);
	//CRLog::trace("isLink : returning");
	return !path.empty() ? (jstring)env->NewGlobalRef(env->NewStringUTF(path.c_str())) : NULL;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    suspendLongOperationInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_suspendLongOperationInternal
  (JNIEnv *, jclass)
{
	_timeoutControl.cancel();
}


#define BUTTON_BACKLIGHT_CONTROL_PATH "/sys/class/leds/button-backlight/brightness"
/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setKeyBacklightInternal
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setKeyBacklightInternal
  (JNIEnv *, jclass, jint n)
{
	FILE * f = fopen(BUTTON_BACKLIGHT_CONTROL_PATH, "wb");
	if (!f)
		return JNI_FALSE;
	fwrite(n ? "1" : "0", 1, 1, f);
	fclose(f);
	return JNI_TRUE;
}

//=====================================================================

static JNINativeMethod sEngineMethods[] = {
  /* name, signature, funcPtr */
  {"initInternal", "([Ljava/lang/String;I)Z", (void*)Java_org_coolreader_crengine_Engine_initInternal},
  {"uninitInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_uninitInternal},
  {"getFontFaceListInternal", "()[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getFontFaceListInternal},
  {"getFontFaceAndFileNameListInternal", "()[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getFontFaceAndFileNameListInternal},
  {"setCacheDirectoryInternal", "(Ljava/lang/String;I)Z", (void*)Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal},
  {"scanBookPropertiesInternal", "(Lorg/coolreader/crengine/FileInfo;)Z", (void*)Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal},
  {"setHyphenationMethod", "(I[B)Z", (void*)Java_org_coolreader_crengine_Engine_setHyphenationMethod},
  {"getArchiveItemsInternal", "(Ljava/lang/String;)[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getArchiveItemsInternal},
  {"isLink", "(Ljava/lang/String;)Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_isLink},
  {"suspendLongOperationInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_suspendLongOperationInternal},
  {"setKeyBacklightInternal", "(I)Z", (void*)Java_org_coolreader_crengine_Engine_setKeyBacklightInternal},
  {"scanBookCoverInternal", "(Ljava/lang/String;)[B", (void*)Java_org_coolreader_crengine_Engine_scanBookCoverInternal},
  {"drawBookCoverInternal", "(Landroid/graphics/Bitmap;[BLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V", (void*)Java_org_coolreader_crengine_Engine_drawBookCoverInternal},
  {"haveFcLangCodeInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_haveFcLangCodeInternal},
  {"checkFontLanguageCompatibilityInternal", "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_checkFontLanguageCompatibilityInternal},
  {"listFilesInternal", "(Ljava/io/File;)[Ljava/io/File;", (void*)Java_org_coolreader_crengine_Engine_listFilesInternal}
};


static JNINativeMethod sDocViewMethods[] = {
  /* name, signature, funcPtr */
  {"createInternal", "()V", (void*)Java_org_coolreader_crengine_DocView_createInternal},
  {"destroyInternal", "()V", (void*)Java_org_coolreader_crengine_DocView_destroyInternal},
  {"getPageImageInternal", "(Landroid/graphics/Bitmap;I)V", (void*)Java_org_coolreader_crengine_DocView_getPageImageInternal},
  {"loadDocumentInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_DocView_loadDocumentInternal},
  {"loadDocumentFromMemoryInternal", "([BLjava/lang/String;)Z", (void*)Java_org_coolreader_crengine_DocView_loadDocumentFromMemoryInternal},
  {"getSettingsInternal", "()Ljava/util/Properties;", (void*)Java_org_coolreader_crengine_DocView_getSettingsInternal},
  {"applySettingsInternal", "(Ljava/util/Properties;)Z", (void*)Java_org_coolreader_crengine_DocView_applySettingsInternal},
  {"setStylesheetInternal", "(Ljava/lang/String;)V", (void*)Java_org_coolreader_crengine_DocView_setStylesheetInternal},
  {"resizeInternal", "(II)V", (void*)Java_org_coolreader_crengine_DocView_resizeInternal},
  {"doCommandInternal", "(II)Z", (void*)Java_org_coolreader_crengine_DocView_doCommandInternal},
  {"getCurrentPageBookmarkInternal", "()Lorg/coolreader/crengine/Bookmark;", (void*)Java_org_coolreader_crengine_DocView_getCurrentPageBookmarkInternal},
  {"goToPositionInternal", "(Ljava/lang/String;Z)Z", (void*)Java_org_coolreader_crengine_DocView_goToPositionInternal},
  {"getPositionPropsInternal", "(Ljava/lang/String;)Lorg/coolreader/crengine/PositionProperties;", (void*)Java_org_coolreader_crengine_DocView_getPositionPropsInternal},
  {"updateBookInfoInternal", "(Lorg/coolreader/crengine/BookInfo;)V", (void*)Java_org_coolreader_crengine_DocView_updateBookInfoInternal},
  {"getTOCInternal", "()Lorg/coolreader/crengine/TOCItem;", (void*)Java_org_coolreader_crengine_DocView_getTOCInternal},
  {"clearSelectionInternal", "()V", (void*)Java_org_coolreader_crengine_DocView_clearSelectionInternal},
  {"findTextInternal", "(Ljava/lang/String;III)Z", (void*)Java_org_coolreader_crengine_DocView_findTextInternal},
  {"setBatteryStateInternal", "(I)V", (void*)Java_org_coolreader_crengine_DocView_setBatteryStateInternal},
  {"getCoverPageDataInternal", "()[B", (void*)Java_org_coolreader_crengine_DocView_getCoverPageDataInternal},
  {"setPageBackgroundTextureInternal", "([BI)V", (void*)Java_org_coolreader_crengine_DocView_setPageBackgroundTextureInternal},
  {"updateSelectionInternal", "(Lorg/coolreader/crengine/Selection;)V", (void*)Java_org_coolreader_crengine_DocView_updateSelectionInternal},
  {"checkLinkInternal", "(III)Ljava/lang/String;", (void*)Java_org_coolreader_crengine_DocView_checkLinkInternal},
  {"goLinkInternal", "(Ljava/lang/String;)I", (void*)Java_org_coolreader_crengine_DocView_goLinkInternal},
  {"moveSelectionInternal", "(Lorg/coolreader/crengine/Selection;II)Z", (void*)Java_org_coolreader_crengine_DocView_moveSelectionInternal},
  {"swapToCacheInternal", "()I", (void*)Java_org_coolreader_crengine_DocView_swapToCacheInternal},
  {"checkImageInternal", "(IILorg/coolreader/crengine/ImageInfo;)Z", (void*)Java_org_coolreader_crengine_DocView_checkImageInternal},
  {"drawImageInternal", "(Landroid/graphics/Bitmap;ILorg/coolreader/crengine/ImageInfo;)Z", (void*)Java_org_coolreader_crengine_DocView_drawImageInternal},
  {"closeImageInternal", "()Z", (void*)Java_org_coolreader_crengine_DocView_closeImageInternal},
  {"hilightBookmarksInternal", "([Lorg/coolreader/crengine/Bookmark;)V", (void*)Java_org_coolreader_crengine_DocView_hilightBookmarksInternal},
  {"checkBookmarkInternal", "(IILorg/coolreader/crengine/Bookmark;)Z", (void*)Java_org_coolreader_crengine_DocView_checkBookmarkInternal},
  {"isRenderedInternal", "()Z", (void*)Java_org_coolreader_crengine_DocView_isRenderedInternal}
};

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
static int jniRegisterNativeMethods(JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    LOGV("Registering %s natives\n", className);
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
   JNIEnv* env = NULL;
   jint res = -1;
 
#ifdef JNI_VERSION_1_6
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_6) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_6\n");
   	    res = JNI_VERSION_1_6;
    }
#endif
#ifdef JNI_VERSION_1_4
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_4\n");
   	    res = JNI_VERSION_1_4;
    }
#endif
#ifdef JNI_VERSION_1_2
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_2) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_2\n");
   	    res = JNI_VERSION_1_2;
    }
#endif
	if ( res==-1 )
		return res;
 
    jniRegisterNativeMethods(env, "org/coolreader/crengine/Engine", sEngineMethods, sizeof(sEngineMethods)/sizeof(JNINativeMethod));
    jniRegisterNativeMethods(env, "org/coolreader/crengine/DocView", sDocViewMethods, sizeof(sDocViewMethods)/sizeof(JNINativeMethod));
    LOGI("JNI_OnLoad: native methods are registered!\n");
    return res;
}
