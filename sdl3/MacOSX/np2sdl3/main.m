/**
 * @file	main.m
 * @brief	メイン
 */

#include "compiler.h"
#include "../../np2.h"
#include "../../dosio.h"

/**
 * メイン
 * @param[in] argc 引数
 * @param[in] argv 引数
 * @return リザルト コード
 */
int main(int argc, char * argv[])
{
    NSString *appSupport = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject];
        NSString *np2Path = [appSupport stringByAppendingPathComponent:@"np2"];
        [[NSFileManager defaultManager] createDirectoryAtPath:np2Path withIntermediateDirectories:YES attributes:nil error:NULL];
        file_setcd([np2Path UTF8String]);
    
  //  NSString *pstrBundlePath = [[NSBundle mainBundle] bundlePath];
//    file_setcd([pstrBundlePath UTF8String]);


	char** q = &argv[1];
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "-psn_", 5) == 0)
		{
		}
		else if (strcasecmp(argv[i], "-NSDocumentRevisionsDebugMode") == 0)
		{
			i++;
		}
		else
		{
			*q++ = argv[i];
		}
	}
	*q = NULL;

	return np2_main((int)(q - argv), argv);
}
