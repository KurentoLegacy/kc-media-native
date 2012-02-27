/**
 * Based on cmdutils.h from FFmpeg 
 *
 * @author Miguel París Díaz
 * 
 */

#ifndef FFMPEG_CMDUTILS_H
#define FFMPEG_CMDUTILS_H

#include <inttypes.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"


extern const char **opt_names;
extern AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
extern AVFormatContext *avformat_opts;
extern struct SwsContext *sws_opts;

/**
 * Fallback for options that are not explicitly handled, these will be
 * parsed through AVOptions.
 */
int opt_default(const char *opt, const char *arg);

/**
 * Parses a string and returns its corresponding value as a double.
 * Exits from the application if the string cannot be correctly
 * parsed or the corresponding value is invalid.
 *
 * @param context the context of the value to be set (e.g. the
 * corresponding commandline option name)
 * @param numstr the string to be parsed
 * @param type the type (OPT_INT64 or OPT_FLOAT) as which the
 * string should be parsed
 * @param min the minimum valid accepted value
 * @param max the maximum valid accepted value
 */
double parse_number_or_die(const char *context, const char *numstr, int type, double min, double max);

/**
 * Parses a string specifying a time and returns its corresponding
 * value as a number of microseconds. Exits from the application if
 * the string cannot be correctly parsed.
 *
 * @param context the context of the value to be set (e.g. the
 * corresponding commandline option name)
 * @param timestr the string to be parsed
 * @param is_duration a flag which tells how to interpret timestr, if
 * not zero timestr is interpreted as a duration, otherwise as a
 * date
 *
 * @see parse_date()
 */
int64_t parse_time_or_die(const char *context, const char *timestr, int is_duration);

typedef struct {
    const char *name;
    int flags;
#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_GRAB   0x0040
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_FUNC2  0x0400
#define OPT_INT64  0x0800
#define OPT_EXIT   0x1000
     union {
        void (*func_arg)(const char *); //FIXME passing error code as int return would be nicer then exit() in the func
        int *int_arg;
        char **str_arg;
        float *float_arg;
        int (*func2_arg)(const char *, const char *);
        int64_t *int64_arg;
    } u;
    const char *help;
    const char *argname;
} OptionDef;


/**
 * Parses the command line arguments.
 * @param options Array with the definitions required to interpret every
 * option of the form: -<option_name> [<argument>]
 * @param parse_arg_function Name of the function called to process every
 * argument without a leading option name flag. NULL if such arguments do
 * not have to be processed.
 */
void parse_options(int argc, char **argv, const OptionDef *options,
                   void (* parse_arg_function)(const char*));

void set_context_opts(void *ctx, void *opts_ctx, int flags);

#endif /* FFMPEG_CMDUTILS_H */
