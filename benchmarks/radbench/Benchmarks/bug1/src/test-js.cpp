#include <pthread.h>

// STL
#include <string>
#include <iostream>

// SpiderMonkey
#include <jsapi.h>

// nspr ( Netscape Portable Runtime )
#include <prinit.h>

#define MY_THREAD_COUNT 2

pthread_barrier_t my_barrier;

JSRuntime *poRt = JS_NewRuntime(0x100000);

JSClass global_class = {
    "Global", 0,
    JS_PropertyStub,  JS_PropertyStub,JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,  JS_FinalizeStub

};

void* RunJavascript( void* pArguments )
{
//	js_InitContextThread(JSContext *cx);
    JSContext* poCtx = JS_NewContext( poRt, 0x1000);

    JS_BeginRequest( poCtx );

    JSObject* poGlobal = JS_NewObject( poCtx, &global_class, NULL, NULL );

    JS_InitStandardClasses( poCtx, poGlobal );

    JS_EndRequest( poCtx );

    // EVALUATE SCRIPT
    jsval rval;

    JS_BeginRequest( poCtx );

    std::string strScript("for( i =0; i < 1; i++ ) {  }; ");

    JSBool ok = JS_EvaluateScript( poCtx, poGlobal, strScript.c_str(),
                                   strScript.length(), "test.js", 1, &rval );

    JS_EndRequest( poCtx );
    pthread_barrier_wait(&my_barrier);
    JS_DestroyContext( poCtx );

    /*std::cout << "test.js eval " << ok << std::endl;
      jsdouble d;
      JS_ValueToNumber(poCtx, rval, &d);
      std::cout << "test.js returns " << d << std::endl; */
    return NULL;
}

int main(int argc, char* argv[])
{
    pthread_barrier_init(&my_barrier, NULL, MY_THREAD_COUNT);
    PR_Init( PR_USER_THREAD, PR_PRIORITY_NORMAL, 0/*dummy*/ );

#if 0
    ACE_thread_t* threadID = new ACE_thread_t[MY_THREAD_COUNT + 1 ];
    ACE_hthread_t* threadHandles = new ACE_hthread_t[MY_THREAD_COUNT + 1];

    if ( ACE_Thread::spawn_n( threadID, MY_THREAD_COUNT,
                              (ACE_THR_FUNC)RunJavascript, NULL,
                              THR_JOINABLE|THR_NEW_LWP,
                              ACE_DEFAULT_THREAD_PRIORITY, 0, 0, threadHandles ) == 1 )
        ACE_OS::printf("bug");

    for( int i = 0; i < MY_THREAD_COUNT; ++i )
        {
            ACE_Thread::join( threadHandles[i] );
        }
#endif

    pthread_t *threadHandles = new pthread_t[MY_THREAD_COUNT];
 
    for(int i=0; i<MY_THREAD_COUNT; ++i)
        if(pthread_create(&threadHandles[i], NULL, RunJavascript, NULL) != 0) {
            perror("pthread_create");
            exit(0);
        }
 
    for(int i=0; i<MY_THREAD_COUNT; ++i)
        pthread_join(threadHandles[i], NULL);

    return 0; 
}
