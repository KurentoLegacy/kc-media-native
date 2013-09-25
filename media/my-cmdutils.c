/*
 * Kurento Android Media: Android Media Library based on FFmpeg.
 * Copyright (C) 2011  Tikal Technologies
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Based on cmdutils.c from FFmpeg 
 *
 * @author Miguel París Díaz
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

/* Include only the enabled headers since some compilers (namely, Sun
   Studio) will not omit unused inline functions and create undefined
   references to libraries that are not being built. */

#include "config.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"
#include "libavutil/avstring.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/opt.h"
#include "my-cmdutils.h"
#if CONFIG_NETWORK
#include "libavformat/network.h"
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

const char **opt_names;
static int opt_name_count;
AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
AVFormatContext *avformat_opts;
struct SwsContext *sws_opts;



double parse_number_or_die(const char *context, const char *numstr, int type, double min, double max)
{
    char *tail;
    const char *error;
    double d = strtod(numstr, &tail);
    if (*tail)
        error= "Expected number for %s but found: %s\n";
    else if (d < min || d > max)
        error= "The value for %s was %s which is not within %f - %f\n";
    else if(type == OPT_INT64 && (int64_t)d != d)
        error= "Expected int64 for %s but found %s\n";
    else
        return d;
    fprintf(stderr, error, context, numstr, min, max);
    exit(1);
}

static const OptionDef* find_option(const OptionDef *po, const char *name){
    while (po->name != NULL) {
        if (!strcmp(name, po->name))
            break;
        po++;
    }
    return po;
}

void parse_options(int argc, char **argv, const OptionDef *options,
                   void (* parse_arg_function)(const char*))
{
    const char *opt, *arg;
    int optindex, handleoptions=1;
    const OptionDef *po;

    /* parse options */
    optindex = 1;
    while (optindex < argc) {
        opt = argv[optindex++];

        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
            int bool_val = 1;
            if (opt[1] == '-' && opt[2] == '\0') {
                handleoptions = 0;
                continue;
            }
            opt++;
            po= find_option(options, opt);
            if (!po->name && opt[0] == 'n' && opt[1] == 'o') {
                /* handle 'no' bool option */
                po = find_option(options, opt + 2);
                if (!(po->name && (po->flags & OPT_BOOL)))
                    goto unknown_opt;
                bool_val = 0;
            }
            if (!po->name)
                po= find_option(options, "default");
            if (!po->name) {
unknown_opt:
                fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], opt);
                exit(1);
            }
            arg = NULL;
            if (po->flags & HAS_ARG) {
                arg = argv[optindex++];
                if (!arg) {
                    fprintf(stderr, "%s: missing argument for option '%s'\n", argv[0], opt);
                    exit(1);
                }
            }
            if (po->flags & OPT_STRING) {
                char *str;
                str = av_strdup(arg);
                *po->u.str_arg = str;
            } else if (po->flags & OPT_BOOL) {
                *po->u.int_arg = bool_val;
            } else if (po->flags & OPT_INT) {
                *po->u.int_arg = parse_number_or_die(opt, arg, OPT_INT64, INT_MIN, INT_MAX);
            } else if (po->flags & OPT_INT64) {
                *po->u.int64_arg = parse_number_or_die(opt, arg, OPT_INT64, INT64_MIN, INT64_MAX);
            } else if (po->flags & OPT_FLOAT) {
                *po->u.float_arg = parse_number_or_die(opt, arg, OPT_FLOAT, -1.0/0.0, 1.0/0.0);
            } else if (po->flags & OPT_FUNC2) {
                if (po->u.func2_arg(opt, arg) < 0) {
                    fprintf(stderr, "%s: failed to set value '%s' for option '%s'\n", argv[0], arg, opt);
                    exit(1);
                }
            } else {
                po->u.func_arg(arg);
            }
            if(po->flags & OPT_EXIT)
                exit(0);
        } else {
            if (parse_arg_function)
                parse_arg_function(opt);
        }
    }
}

int opt_default(const char *opt, const char *arg){
    int type;
    int ret= 0;
    const AVOption *o= NULL;
    int opt_types[]={AV_OPT_FLAG_VIDEO_PARAM, AV_OPT_FLAG_AUDIO_PARAM, 0, AV_OPT_FLAG_SUBTITLE_PARAM, 0};

    for(type=0; type<AVMEDIA_TYPE_NB && ret>= 0; type++){
        const AVOption *o2 = av_find_opt(avcodec_opts[0], opt, NULL, opt_types[type], opt_types[type]);
        if(o2)
            ret = av_set_string3(avcodec_opts[type], opt, arg, 1, &o);
    }
    if(!o)
        ret = av_set_string3(avformat_opts, opt, arg, 1, &o);
    if(!o && sws_opts)
        ret = av_set_string3(sws_opts, opt, arg, 1, &o);
    if(!o){
        if(opt[0] == 'a')
            ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_AUDIO], opt+1, arg, 1, &o);
        else if(opt[0] == 'v')
            ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_VIDEO], opt+1, arg, 1, &o);
        else if(opt[0] == 's')
            ret = av_set_string3(avcodec_opts[AVMEDIA_TYPE_SUBTITLE], opt+1, arg, 1, &o);
    }
    if (o && ret < 0) {
        fprintf(stderr, "Invalid value '%s' for option '%s'\n", arg, opt);
        exit(1);
    }
    if (!o) {
        fprintf(stderr, "Unrecognized option '%s'\n", opt);
        exit(1);
    }

//    av_log(NULL, AV_LOG_ERROR, "%s:%s: %f 0x%0X\n", opt, arg, av_get_double(avcodec_opts, opt, NULL), (int)av_get_int(avcodec_opts, opt, NULL));

    //FIXME we should always use avcodec_opts, ... for storing options so there will not be any need to keep track of what i set over this
    opt_names= av_realloc(opt_names, sizeof(void*)*(opt_name_count+1));
    opt_names[opt_name_count++]= o->name;

    if(avcodec_opts[0]->debug || avformat_opts->debug)
        av_log_set_level(AV_LOG_DEBUG);
    return 0;
}


void set_context_opts(void *ctx, void *opts_ctx, int flags)
{
    int i;
    for(i=0; i<opt_name_count; i++){
        char buf[256];
        const AVOption *opt;
        const char *str= av_get_string(opts_ctx, opt_names[i], &opt, buf, sizeof(buf));
        /* if an option with name opt_names[i] is present in opts_ctx then str is non-NULL */
        if(str && ((opt->flags & flags) == flags))
            av_set_string3(ctx, opt_names[i], str, 1, NULL);
    }
}

