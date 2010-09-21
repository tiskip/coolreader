// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>


#include <stdio.h>
#include <stdlib.h>

#include "org_coolreader_crengine_Engine.h"
#include "org_coolreader_crengine_ReaderView.h"

#include "cr3java.h"
#include "readerview.h"
#include "crengine.h"


#include <fb2def.h>

#define XS_IMPLEMENT_SCHEME 1
#include <fb2def.h>
#include <sys/stat.h>


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
};

static bool GetBookProperties(const char *name,  BookProperties * pBookProps)
{
    CRLog::trace("GetBookProperties( %s )", name);

    // open stream
    LVStreamRef stream = LVOpenFileStream(name, LVOM_READ);
    if (!stream) {
        CRLog::error("cannot open file %s", name);
        return false;
    }
    // check archieve
#ifdef USE_ZLIB
    LVContainerRef arc;
    //printf("start opening arc\n");
    //for ( int i=0; i<1000; i++ )
    //for ( int kk=0; kk<1000; kk++) 
    {
        arc = LVOpenArchieve( stream );
    //printf("end opening arc\n");
    if (!arc.isNull())
    {
        CRLog::trace("%s is archive with %d items", name, arc->GetObjectCount());
        // archieve
        const LVContainerItemInfo * bestitem = NULL;
        const LVContainerItemInfo * fb2item = NULL;
        const LVContainerItemInfo * fbditem = NULL;
        for (int i=0; i<arc->GetObjectCount(); i++)
        {
            const LVContainerItemInfo * item = arc->GetObjectInfo(i);
            if (item)
            {
                if ( !item->IsContainer() )
                {
                    lString16 name( item->GetName() );
                    if ( name.length() > 5 )
                    {
                        name.lowercase();
                        const lChar16 * pext = name.c_str() + name.length() - 4;
                        if ( pext[0]=='.' && pext[1]=='f' && pext[2]=='b' && pext[3]=='2') {
                            fb2item = item;
                        } else if ( pext[0]=='.' && pext[1]=='f' && pext[2]=='b' && pext[3]=='d') {
                            fbditem = item;
                        }
                    }
                }
            }
        }
        bestitem = fb2item;
        if ( fbditem )
            bestitem = fbditem;
        if ( !bestitem )
            return false;
        CRLog::trace( "opening item %s from archive", UnicodeToUtf8(bestitem->GetName()).c_str() );
        //printf("start opening stream\n");
        //for ( int k=0; k<1000; k++ ) {
            stream = arc->OpenStream( bestitem->GetName(), LVOM_READ );
            char buf[8192];
            stream->Read(buf, 8192, NULL );
        //}
        //printf("end opening stream\n");
        if ( stream.isNull() )
            return false;
        CRLog::trace( "stream created" );
        // opened archieve stream
    }
    }

#endif //USE_ZLIB

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
    lString16 authors = extractDocAuthors( &doc );
    lString16 title = extractDocTitle( &doc );
    lString16 series = extractDocSeriesReverse( &doc, pBookProps->seriesNumber );
#if SERIES_IN_AUTHORS==1
    if ( !series.empty() )
    	authors << L"    " << series;
#endif
    pBookProps->title = title;
    pBookProps->author = authors;
    pBookProps->series = series;
    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    struct stat fs;
    time_t t;
    if ( stat( name, &fs ) ) {
        t = (time_t)time(0);
    } else {
        t = fs.st_mtime;
    }
    pBookProps->filedate = getDateTimeString( t );
    return true;
}




/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    scanBookPropertiesInternal
 * Signature: (Lorg/coolreader/crengine/FileInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal
  (JNIEnv * _env, jobject _view, jobject _fileInfo)
{
	CRJNIEnv env(_env);
	jclass objclass = env->GetObjectClass(_fileInfo);
	jfieldID fid = env->GetFieldID(objclass, "pathname", "Ljava/lang/String;");
	lString16 filename = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );

	if ( filename.empty() )
		return JNI_FALSE;

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
	
	return JNI_TRUE;
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
	static char str[1001];
	snprintf(str, 1000, "CoolReader Fatal Error #%d: %s", errorCode, errorText);
	LOGE(str);
	LOGASSERTFAILED(errorText, str);
}

/// set fatal error handler
void crSetFatalErrorHandler( lv_FatalErrorHandler_t * handler );

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    initInternal
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_initInternal
  (JNIEnv * penv, jobject obj, jobjectArray fontArray)
{
	CRJNIEnv env(penv);
	
	LOGI("initInternal called");
	// set fatal error handler
	crSetFatalErrorHandler( &cr3androidFatalErrorHandler );
	LOGD("Redirecting CDRLog to Android");
	CRLog::setLogger( new JNICDRLogger() );
	CRLog::setLogLevel( CRLog::LL_TRACE );
	CRLog::info("CREngine log redirected");
	CRLog::info("creating font manager");
	
	InitFontManager(lString8());
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
	CRLog::info("%d fonts registered", (int)fontMan->GetFontCount());
	return fontMan->GetFontCount() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    uninitInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_uninitInternal
  (JNIEnv *, jobject)
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
  (JNIEnv * penv, jobject obj)
{
	LOGI("getFontFaceListInternal called");
	CRJNIEnv env(penv);
	lString16Collection list;
	fontMan->getFaceList(list);
	return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setCacheDirectoryInternal
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal
  (JNIEnv * penv, jobject obj, jstring dir, jint size)
{
	CRJNIEnv env(penv);
	bool res = ldomDocCache::init(env.fromJavaString(dir), size ); 
	return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setHyphenationDirectoryInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setHyphenationDirectoryInternal
  (JNIEnv * penv, jobject obj, jstring dir)
{
	CRJNIEnv env(penv);
	bool res = HyphMan::initDictionaries(env.fromJavaString(dir)); 
	return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getHyphenationDictionaryListInternal
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getHyphenationDictionaryListInternal
  (JNIEnv * penv, jobject obj)
{
	LOGI("getHyphenationDictionaryListInternal called");
	CRJNIEnv env(penv);
	HyphDictionaryList * plist = HyphMan::getDictList();
	lString16Collection list;
	if ( plist ) {
		for ( int i=0; i<plist->length(); i++ ) {
			HyphDictionary * dict = plist->get(i);
			list.add( dict->getTitle() );
		}
	}
	return env.toJavaStringArray(list);
}


//=====================================================================

static JNINativeMethod sEngineMethods[] = {
  {"initInternal", "([Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_initInternal},
  {"uninitInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_uninitInternal},
  {"getFontFaceListInternal", "()[Ljava/lang/String", (void*)Java_org_coolreader_crengine_Engine_getFontFaceListInternal},
  {"setCacheDirectoryInternal", "(Ljava/lang/String;I)Z", (void*)Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal},
  {"setHyphenationDirectoryInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_setHyphenationDirectoryInternal},
  {"getHyphenationDictionaryListInternal", "()[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getHyphenationDictionaryListInternal},
  {"scanBookPropertiesInternal", "(Lorg/coolreader/crengine/FileInfo;)Z", (void*)Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal},
};


static JNINativeMethod sReaderViewMethods[] = {
  /* name, signature, funcPtr */
  {"createInternal", "()V", (void*)Java_org_coolreader_crengine_ReaderView_createInternal},
  {"destroyInternal", "()V", (void*)Java_org_coolreader_crengine_ReaderView_destroyInternal},
  {"getPageImage", "(Landroid/graphics/Bitmap;)V", (void*)Java_org_coolreader_crengine_ReaderView_getPageImage},
  {"loadDocument", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_ReaderView_loadDocumentInternal},
  {"getSettings", "()Ljava/lang/String;", (void*)Java_org_coolreader_crengine_ReaderView_getSettingsInternal},
  {"applySettings", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_ReaderView_applySettingsInternal},
  {"readHistory", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_ReaderView_readHistoryInternal},
  {"writeHistory", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_ReaderView_writeHistoryInternal},
  {"setStylesheet", "(Ljava/lang/String;)V", (void*)Java_org_coolreader_crengine_ReaderView_setStylesheetInternal},
  {"resize", "(II)V", (void*)Java_org_coolreader_crengine_ReaderView_resizeInternal},
  {"doCommand", "(II)Z", (void*)Java_org_coolreader_crengine_ReaderView_doCommandInternal},
  {"getState", "()Lorg/coolreader/crengine/ReaderView/DocumentInfo;", (void*)Java_org_coolreader_crengine_ReaderView_getStateInternal},
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
   jint result = -1;
 
   if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      return result;
   }
 
   jniRegisterNativeMethods(env, "org/coolreader/crengine/Engine", sEngineMethods, 1);
   jniRegisterNativeMethods(env, "org/coolreader/crengine/ReaderView", sReaderViewMethods, 1);
   
   return JNI_VERSION_1_4;
}
