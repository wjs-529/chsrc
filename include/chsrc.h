/** ------------------------------------------------------------
 * SPDX-License-Identifier: GPL-3.0-or-later
 * -------------------------------------------------------------
 * File Name     : chsrc.h
 * File Authors  : Aoran Zeng <ccmywish@qq.com>
 *               |  Heng Guo  <2085471348@qq.com>
 * Contributors  :  Peng Gao  <gn3po4g@outlook.com>
 *               |
 * Created on    : <2023-08-29>
 * Last modified : <2024-08-18>
 *
 * chsrc 头文件
 * ------------------------------------------------------------*/

#include "xy.h"
#include "source.h"

#define App_Name "chsrc"

/* 命令行选项 */
bool CliOpt_IPv6 = false;
bool CliOpt_Locally = false;
bool CliOpt_InEnglish = false;
bool CliOpt_DryRun = false;

/**
 * -local 的含义是启用 *项目级* 换源
 *
 * 每个 target 对 [-local] 的支持情况可使用 | chsrc ls <target> | 来查看
 *
 *
 * 1. 默认不使用该选项时，含义是 *全局* 换源，
 *
 *    全局分为 (1)系统级 (2)用户级
 *
 *    大多数第三方配置软件往往默认进行的是 *用户级* 的配置。所以 chsrc 首先将尝试使用 *用户级* 配置
 *
 * 2. 若不存在 *用户级* 的配置，chsrc 将采用 *系统级* 的配置
 *
 * 3. 最终效果本质由第三方软件决定，如 poetry 默认实现的就是项目级的换源
 */

#define Exit_UserCause    1
#define Exit_Unsupported  2
#define Exit_MatinerIssue 3
#define Exit_FatalBug     4
#define Exit_FatalUnkownError  5

#define chsrc_log(str)   xy_log(App_Name,str)
#define chsrc_succ(str)  xy_succ(App_Name,str)
#define chsrc_info(str)  xy_info(App_Name,str)
#define chsrc_warn(str)  xy_warn(App_Name,str)
#define chsrc_error(str) xy_error(App_Name,str)

// 2系列都是带有括号的
#define chsrc_succ2(str)  xy_succ_brkt(App_Name,"成功",str)
#define chsrc_log2(str)   xy_info_brkt(App_Name,"LOG",str)
#define chsrc_warn2(str)  xy_warn_brkt(App_Name,"警告",str)
#define chsrc_error2(str) xy_error_brkt(App_Name,"错误",str)

#define to_red(str)        xy_str_to_red(str)
#define to_blue(str)       xy_str_to_blue(str)
#define to_green(str)      xy_str_to_green(str)
#define to_yellow(str)     xy_str_to_yellow(str)
#define to_purple(str)     xy_str_to_purple(str)
#define to_bold(str)       xy_str_to_bold(str)
#define to_boldred(str)    xy_str_to_bold(xy_str_to_red(str))
#define to_boldblue(str)   xy_str_to_bold(xy_str_to_blue(str))
#define to_boldgreen(str)  xy_str_to_bold(xy_str_to_green(str))
#define to_boldyellow(str) xy_str_to_bold(xy_str_to_yellow(str))
#define to_boldpurple(str) xy_str_to_bold(xy_str_to_purple(str))

void
chsrc_note2 (const char* str)
{
  xy_log_brkt (to_yellow (App_Name), to_boldyellow ("提示"), to_yellow (str));
}

#define YesMark "✓"
#define NoMark "x"
#define SemiYesMark "⍻"

/**
 * @translation Done
 */
void
log_check_result (const char *check_what, const char *check_type, bool exist)
{
  char *chk_msg       = NULL;
  char *not_exist_msg = NULL;
  char *exist_msg     = NULL;

  if (CliOpt_InEnglish)
    {
      chk_msg       = "CHECK";
      not_exist_msg = " doesn't exist";
      exist_msg     = " exists";
    }
  else
    {
      chk_msg       = "检查";
      not_exist_msg = " 不存在";
      exist_msg     = " 存在";
    }


  if (!exist)
    {
      xy_log_brkt (App_Name, to_boldred (chk_msg), xy_strjoin (5,
                   to_red (NoMark " "), check_type, " ", to_red (check_what), not_exist_msg));
    }
  else
    {
      xy_log_brkt (App_Name, to_boldgreen (chk_msg), xy_strjoin (5,
                   to_green (YesMark " "), check_type, " ", to_green (check_what), exist_msg));
    }
}


/**
 * @translation Done
 */
void
log_cmd_result (bool result, int ret_code)
{
  char *run_msg  = NULL;
  char *succ_msg = NULL;
  char *fail_msg = NULL;

  if (CliOpt_InEnglish)
    {
      run_msg  = "RUN";
      succ_msg = YesMark " executed successfully";
      fail_msg = NoMark  " executed unsuccessfully, return code ";
    }
  else
    {
      run_msg  = "运行";
      succ_msg = YesMark " 命令执行成功";
      fail_msg = NoMark  " 命令执行失败，返回码 ";
    }

  if (result)
    xy_log_brkt (to_green (App_Name), to_boldgreen (run_msg), to_green (succ_msg));
  else
    {
      char buf[8] = {0};
      sprintf (buf, "%d", ret_code);
      char *log = xy_2strjoin (to_red (fail_msg), to_boldred (buf));
      xy_log_brkt (to_red (App_Name), to_boldred (run_msg), log);
    }
}



bool
is_url (const char *str)
{
  return (xy_str_start_with (str, "http://") || xy_str_start_with (str, "https://"));
}


/**
 * 检测二进制程序是否存在
 *
 * @param  check_cmd  检测 `prog_name` 是否存在的一段命令，一般来说，填 `prog_name` 本身即可，
 *                    但是某些情况下，需要使用其他命令绕过一些特殊情况，比如 python 这个命令在Windows上
 *                    会自动打开 Microsoft Store，需避免
 *
 * @param  prog_name   要检测的二进制程序名
 *
 * @translation Done
 */
bool
query_program_exist (char *check_cmd, char *prog_name)
{
  char *which = check_cmd;

  int ret = system(which);

  // char buf[32] = {0}; sprintf(buf, "错误码: %d", ret);

  char *msg = CliOpt_InEnglish ? "command" : "命令";

  if (0 != ret)
    {
      // xy_warn (xy_strjoin(4, "× 命令 ", progname, " 不存在，", buf));
      log_check_result (prog_name, msg, false);
      return false;
    }
  else
    {
      log_check_result (prog_name, msg, true);
      return true;
    }
}


/**
 * @note 此函数只能对接受 --version 选项的程序有效
 */
bool
chsrc_check_program (char *prog_name)
{
  char *quiet_cmd = xy_str_to_quietcmd (xy_2strjoin (prog_name, " --version"));
  return query_program_exist (quiet_cmd, prog_name);
}


/**
 * @note 此函数具有强制性，检测不到就直接退出
 */
void
chsrc_ensure_program (char *prog_name)
{
  char *quiet_cmd = xy_str_to_quietcmd (xy_2strjoin (prog_name, " --version"));
  bool exist = query_program_exist (quiet_cmd, prog_name);
  if (exist)
    {
      // OK, nothing should be done
    }
  else
    {
      chsrc_error (xy_strjoin (3, "未找到 ", prog_name, " 命令，请检查是否存在"));
      exit (Exit_UserCause);
    }
}


bool
chsrc_check_file (char *path)
{
  if (xy_file_exist (path))
    {
      log_check_result (path, "文件", true);
      return true;
    }
  else
    {
      log_check_result (path, "文件", false);
      return false;
    }
}


/**
 * 用于 _setsrc 函数，检测用户输入的镜像站code，是否存在于该target可用源中
 *
 * @param  target  目标名
 * @param  input   如果用户输入 default 或者 def，则选择第一个源
 */
#define find_mirror(s, input) query_mirror_exist(s##_sources, s##_sources_n, (char*)#s+3, input)
int
query_mirror_exist (SourceInfo *sources, size_t size, char *target, char *input)
{
  if (is_url (input))
    {
      chsrc_error ("暂不支持对该软件使用用户自定义源，请联系开发者询问原因或请求支持");
      exit (Exit_Unsupported);
    }

  if (0==size || 1==size)
    {
      chsrc_error (xy_strjoin (3, "当前 ", target, " 无任何可用源，请联系维护者"));
      exit (Exit_MatinerIssue);
    }

  if (2==size)
    {
      chsrc_succ (xy_strjoin (4, sources[1].mirror->name, " 是 ", target, " 目前唯一可用镜像站，感谢他们的慷慨支持"));
    }

  if (xy_streql ("reset", input))
    {
      puts ("将重置为上游默认源");
      return 0; // 返回第1个，因为第1个是上游默认源
    }

  if (xy_streql ("first", input))
    {
      puts ("将使用维护团队测速第一的源");
      return 1; // 返回第2个，因为第1个是上游默认源
    }

  int idx = 0;
  SourceInfo source = sources[0];

  bool exist = false;
  for (int i=0; i<size; i++)
    {
      source = sources[i];
      if (xy_streql (source.mirror->code, input))
        {
          idx = i;
          exist = true;
          break;
        }
    }
  if (!exist)
    {
      chsrc_error (xy_strjoin (3, "镜像站 ", input, " 不存在"));
      chsrc_error (xy_2strjoin ("查看可使用源，请使用 chsrc list ", target));
      exit (Exit_UserCause);
    }
  return idx;
}


/**
 * 该函数来自 oh-my-mirrorz.py，由 @ccmywish 翻译为C语言，但功劳和版权属于原作者
 */
char *
to_human_readable_speed (double speed)
{
  char *scale[] = {"Byte/s", "KByte/s", "MByte/s", "GByte/s", "TByte/s"};
  int i = 0;
  while (speed > 1024.0)
  {
    i += 1;
    speed /= 1024.0;
  }
  char *buf = xy_malloc0 (64);
  sprintf (buf, "%.2f %s", speed, scale[i]);

  char *new = NULL;
  if (i <= 1 ) new = to_red (buf);
  else
    {
      if (i == 2 && speed < 2.00) new = to_yellow (buf);
      else new = to_green (buf);
    }
  return new;
}


/**
 * 测速代码参考自 https://github.com/mirrorz-org/oh-my-mirrorz/blob/master/oh-my-mirrorz.py
 * 功劳和版权属于原作者，由 @ccmywish 修改为C语言，并做了额外调整
 *
 * @return 返回测得的速度，若出错，返回-1
 */
double
measure_speed (const char *url)
{
  char *time_sec = "6";

  /* 现在我们切换至跳转后的链接来测速，不再使用下述判断
  if (xy_str_start_with(url, "https://registry.npmmirror"))
    {
      // 这里 npmmirror 跳转非常慢，需要1~3秒，所以我们给它留够至少8秒测速时间，否则非常不准
      time_sec = "10";
    }
  */

  char *ipv6 = ""; // 默认不启用

  if (CliOpt_IPv6==true)
    {
      ipv6 = "--ipv6";
    }

  // 我们用 —L，因为Ruby China源会跳转到其他地方
  // npmmirror 也会跳转
  char *curl_cmd = xy_strjoin (7, "curl -qsL ", ipv6,
                                  " -o " xy_os_devnull,
                                  " -w \"%{http_code} %{speed_download}\" -m", time_sec,
                                  " -A chsrc/" Chsrc_Banner_Version "  ", url);

  // chsrc_info (xy_2strjoin ("测速命令 ", curl_cmd));

  char *buf = xy_run (curl_cmd, 0, NULL);
  // 如果尾部有换行，删除
  buf = xy_str_strip (buf);

  // 分隔两部分数据
  char *split = strchr (buf, ' ');
  if (split) *split = '\0';

  // puts(buf); puts(split+1);
  int http_code = atoi (buf);
  double speed  = atof (split+1);
  char *speedstr = to_human_readable_speed (speed);

  if (200!=http_code)
    {
      char* httpcodestr = to_yellow (xy_2strjoin ("HTTP码 ", buf));
      puts (xy_strjoin (3, speedstr, " | ",  httpcodestr));
    }
  else
    {
      puts (speedstr);
    }
  return speed;
}


int
get_max_ele_idx_in_dbl_ary (double *array, int size)
{
  double maxval = array[0];
  int maxidx = 0;

  for (int i=1; i<size; i++)
    {
      if (array[i]>maxval)
        {
          maxval = array[i];
          maxidx = i;
        }
    }
  return maxidx;
}

/**
 * 自动测速选择镜像站和源
 *
 * @translation Done
 */
#define auto_select(s) auto_select_(s##_sources, s##_sources_n, (char*)#s+3)
int
auto_select_ (SourceInfo *sources, size_t size, const char *target_name)
{
  if (0==size || 1==size)
    {
      char *msg1 = CliOpt_InEnglish ? "Currently " : "当前 ";
      char *msg2 = CliOpt_InEnglish ? "No any source, please contact maintainers: chsrc issue" : " 无任何可用源，请联系维护者: chsrc issue";
      chsrc_error (xy_strjoin (3, msg1, target_name, msg2));
      exit (Exit_MatinerIssue);
    }

  if (CliOpt_DryRun)
    {
      return 1; // Dry Run 时，跳过测速
    }

  bool only_one = false;
  if (2==size) only_one = true;

  char *check_curl = xy_str_to_quietcmd ("curl --version");
  bool  exist_curl = query_program_exist (check_curl, "curl");
  if (!exist_curl)
    {
      char *msg = CliOpt_InEnglish ? "No curl, unable to measure speed" : "没有curl命令，无法测速";
      chsrc_error (msg);
      exit (Exit_UserCause);
    }

  double speeds[size];
  double speed = 0.0;
  for (int i=0; i<size; i++)
    {
      SourceInfo src = sources[i];
      const char* url = src.mirror->__bigfile_url;
      if (NULL==url)
        {
          if (xy_streql ("upstream", src.mirror->code))
            {
              continue; /* 上游默认源不测速 */
            }
          else
            {
              char *msg1 = CliOpt_InEnglish ? "Dev team doesn't offer " : "开发者未提供 ";
              char *msg2 = CliOpt_InEnglish ? " mirror site's speed measurement link,so skip it" : " 镜像站测速链接，跳过该站点";
              chsrc_warn (xy_strjoin (3, msg1, src.mirror->code, msg2));
              speed = 0;
            }
        }
      else
        {
          char *test_msg = CliOpt_InEnglish ? "Measure speed> " : "测速 ";
          printf ("%s", xy_strjoin (3, test_msg, src.mirror->site , " ... "));

          fflush (stdout);
          speed = measure_speed (url);
        }
      speeds[i] = speed;
    }

  int fast_idx = get_max_ele_idx_in_dbl_ary (speeds, size);

  if (only_one)
    {
      char *is = CliOpt_InEnglish ? " is " : " 是 ";
      char *only_msg = CliOpt_InEnglish ? "the ONLY mirror available currently, thanks for their generous support" : \
                                          " 目前唯一可用镜像站，感谢他们的慷慨支持";
      chsrc_succ (xy_strjoin (4, sources[fast_idx].mirror->name, is, target_name, only_msg));
    }
  else
    {
      char *fast_msg = CliOpt_InEnglish ? "FASTEST mirror site: " : "最快镜像站: ";
      say (xy_2strjoin (fast_msg, to_green (sources[fast_idx].mirror->name)));
    }


  return fast_idx;
}


#define use_specific_mirror_or_auto_select(input, s) \
  (NULL!=(input)) ? find_mirror(s, input) : auto_select(s)



bool
source_is_upstream (SourceInfo *source)
{
  return xy_streql (source->mirror->code, "upstream");
}

bool
source_is_userdefine (SourceInfo *source)
{
  return xy_streql (source->mirror->code, "user");
}

bool
source_has_empty_url (SourceInfo *source)
{
  return source->url == NULL;
}

/**
 * 用户*只可能*通过下面三种方式来换源，无论哪一种都会返回一个 SourceInfo 出来
 *
 * 1. 用户指定 MirrorCode
 * 2. 用户什么都没指定 (将测速选择最快镜像)
 * 3. 用户给了一个 URL
 *
 * @dependency 变量 option
 * @dependency 变量 source
 */
#define chsrc_yield_source(for_what) \
  SourceInfo source; \
  if (is_url (option)) \
    { \
      SourceInfo __tmp = { &UserDefine, option }; \
      source = __tmp; \
    } \
  else \
    { \
      int __index = use_specific_mirror_or_auto_select (option, for_what); \
      source = for_what##_sources[__index]; \
    }



#define split_between_source_changing_process   puts ("--------------------------------")

/**
 * 用于 _setsrc 函数
 *
 * 1. 告知用户选择了什么源和镜像
 * 2. 对选择的源和镜像站进行一定的校验
 *
 * @translation Done
 */
#define chsrc_confirm_source confirm_source(&source)
void
confirm_source (SourceInfo *source)
{
  // 由于实现问题，我们把本应该独立出去的默认上游源，也放在了可以换源的数组中，而且放在第一个
  // chsrc 已经规避用户使用未实现的 `chsrc reset`
  // 但是某些用户可能摸索着强行使用 chsrc set target upstream，从而执行起该禁用的功能，
  // 之所以禁用，是因为有的 reset 我们并没有实现，我们在这里阻止这些邪恶的用户
  if (source_is_upstream (source) && source_has_empty_url (source))
    {
      char *msg = CliOpt_InEnglish ? "Not implement RESET for the target yet" : "暂未对该目标实现重置";
      chsrc_error (msg);
      exit (Exit_Unsupported);
    }
  else if (source_has_empty_url (source))
    {
      char *msg = CliOpt_InEnglish ? "URL of the source doesn't exist, please report a bug to the dev team" : \
                                     "该源URL不存在，请向开发团队提交bug";
      chsrc_error (msg);
      exit (Exit_FatalBug);
    }
  else
    {
      char *msg = CliOpt_InEnglish ? "SELECT  mirror site: " : "选中镜像站: ";
      say (xy_strjoin (5, msg, to_green (source->mirror->abbr), " (", to_green (source->mirror->code), ")"));
    }

  split_between_source_changing_process;
}

#define chsrc_yield_source_and_confirm(for_what) chsrc_yield_source(for_what);chsrc_confirm_source


#define ChsrcTypeAuto     "auto"
#define ChsrcTypeReset    "reset"
#define ChsrcTypeSemiAuto "semiauto"
#define ChsrcTypeManual   "manual"
#define ChsrcTypeUntested "untested"

#define MSG_EN_PUBLIC_URL "If the URL you specify is a public service, you are invited to contribute: chsrc issue"
#define MSG_CN_PUBLIC_URL "若您指定的URL为公有服务，邀您参与贡献: chsrc issue"

#define MSG_EN_FULLY_AUTO "Fully-Auto changed source. "
#define MSG_CN_FULLY_AUTO "全自动换源完成"

#define MSG_EN_SEMI_AUTO  "Semi-Auto changed source. "
#define MSG_CN_SEMI_AUTO  "半自动换源完成"

#define MSG_EN_THANKS     "Thanks to the mirror site: "
#define MSG_CN_THANKS     "感谢镜像提供方: "

#define MSG_EN_BETTER     "If you have a better source changing method , please help: chsrc issue"
#define MSG_CN_BETTER     "若您有更好的换源方案，邀您帮助: chsrc issue"

#define MSG_EN_CONSTRAINT "Implementation constraints require manual operation according to the above prompts. "
#define MSG_CN_CONSTRAINT "因实现约束需按上述提示手工操作"

#define MSG_EN_STILL      "Still need to operate manually according to the above prompts. "
#define MSG_CN_STILL      "仍需按上述提示手工操作"

/**
 * @param source    可为NULL
 * @param last_word 5种选择：ChsrcTypeAuto | ChsrcTypeReset | ChsrcTypeSemiAuto | ChsrcTypeManual | ChsrcTypeUntested
 * @translation Done
 */
void
chsrc_conclude (SourceInfo *source, const char *last_word)
{
  split_between_source_changing_process;

  if (xy_streql (ChsrcTypeAuto, last_word))
    {
      if (source)
        {
          if (source_is_userdefine (source))
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_FULLY_AUTO      MSG_EN_PUBLIC_URL \
                                           : MSG_CN_FULLY_AUTO ", " MSG_CN_PUBLIC_URL;
              chsrc_log (msg);
            }
          else
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_FULLY_AUTO      MSG_EN_THANKS \
                                           : MSG_CN_FULLY_AUTO ", " MSG_CN_THANKS;
              chsrc_log (xy_2strjoin (msg, to_purple (source->mirror->name)));
            }
        }
      else
        {
          char *msg = CliOpt_InEnglish ? MSG_EN_FULLY_AUTO : MSG_CN_FULLY_AUTO;
          chsrc_log (msg);
        }
    }
  else if (xy_streql (ChsrcTypeReset, last_word))
    {
      // source_is_upstream (source)
      char *msg = CliOpt_InEnglish ? "Has been reset to the upstream default source" : "已重置为上游默认源";
      chsrc_log (to_purple (msg));
    }
  else if (xy_streql (ChsrcTypeSemiAuto, last_word))
    {
      if (source)
        {
          if (source_is_userdefine (source))
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_SEMI_AUTO      MSG_EN_STILL      MSG_EN_PUBLIC_URL \
                                           : MSG_CN_SEMI_AUTO ", " MSG_CN_STILL "。" MSG_CN_PUBLIC_URL;
              chsrc_log (msg);
            }
          else
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_SEMI_AUTO      MSG_EN_STILL      MSG_EN_THANKS \
                                           : MSG_CN_SEMI_AUTO ", " MSG_CN_STILL "。" MSG_CN_THANKS;
              chsrc_log (xy_2strjoin (msg, to_purple (source->mirror->name)));
            }
        }
      else
        {
          char *msg = CliOpt_InEnglish ? MSG_EN_SEMI_AUTO      MSG_EN_STILL \
                                       : MSG_CN_SEMI_AUTO ", " MSG_CN_STILL;
          chsrc_log (msg);
        }

      char *msg = CliOpt_InEnglish ? MSG_EN_BETTER : MSG_CN_BETTER;
      chsrc_warn (msg);
    }
  else if (xy_streql (ChsrcTypeManual, last_word))
    {
      if (source)
        {
          if (source_is_userdefine (source))
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_CONSTRAINT      MSG_EN_PUBLIC_URL \
                                           : MSG_CN_CONSTRAINT "; " MSG_CN_PUBLIC_URL;
              chsrc_log (msg);
            }
          else
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_CONSTRAINT      MSG_EN_THANKS \
                                           : MSG_CN_CONSTRAINT ", " MSG_CN_THANKS;
              chsrc_log (xy_2strjoin (msg, to_purple (source->mirror->name)));
            }
        }
      else
        {
          char *msg = CliOpt_InEnglish ? MSG_EN_CONSTRAINT : MSG_CN_CONSTRAINT;
          chsrc_log (msg);
        }
      char *msg = CliOpt_InEnglish ? MSG_EN_BETTER : MSG_CN_BETTER;
      chsrc_warn (msg);
    }
  else if (xy_streql (ChsrcTypeUntested, last_word))
    {
      if (source)
        {
          if (source_is_userdefine (source))
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_PUBLIC_URL : MSG_CN_PUBLIC_URL;
              chsrc_log (msg);
            }
          else
            {
              char *msg = CliOpt_InEnglish ? MSG_EN_THANKS : MSG_CN_THANKS;
              chsrc_log (xy_2strjoin (msg, to_purple (source->mirror->name)));
            }
        }
      else
        {
          char *msg = CliOpt_InEnglish ? "Auto changed source" : "自动换源完成";
          chsrc_log (msg);
        }

      char *msg = CliOpt_InEnglish ? "The method hasn't been tested or has any feedback, please report usage: chsrc issue" : "该换源步骤已实现但未经测试或存在任何反馈，请报告使用情况: chsrc issue";
      chsrc_warn (msg);
    }
  else
    {
      say (last_word);
    }
}



void
chsrc_ensure_root ()
{
  char *euid = getenv ("$EUID");
  if (NULL==euid)
    {
      char *buf = xy_run ("id -u", 0, NULL);
      if (0!=atoi(buf)) goto not_root;
      else return;
    }
  else
    {
      if (0!=atoi(euid)) goto not_root;
      else return;
    }
not_root:
  chsrc_error ("请在命令前使用 sudo 或切换为root用户来保证必要的权限");
  exit (Exit_UserCause);
}


#define RunOpt_Default                0x0000  // 默认若命令运行失败，直接退出
#define RunOpt_Dont_Notify_On_Success 0x0010  // 运行成功不提示用户，只有运行失败时才提示用户
#define RunOpt_No_Last_New_Line       0x0100  // 不输出最后的空行
#define RunOpt_Dont_Abort_On_Failure  0x1000  // 命令运行失败也不退出

static void
chsrc_run (const char *cmd, int run_option)
{
  if (CliOpt_InEnglish)
    xy_log_brkt (to_blue (App_Name), to_boldblue ("RUN"), to_blue (cmd));
  else
    xy_log_brkt (to_blue (App_Name), to_boldblue ("运行"), to_blue (cmd));

  if (CliOpt_DryRun)
    {
      return; // Dry Run 此时立即结束，并不真正执行
    }

  int status = system (cmd);
  if (0==status)
    {
      if (! (RunOpt_Dont_Notify_On_Success & run_option))
        {
          log_cmd_result (true, status);
        }
    }
  else
    {
      log_cmd_result (false, status);
      if (! (run_option & RunOpt_Dont_Abort_On_Failure))
        {
          chsrc_error ("关键错误，强制结束");
          exit (Exit_FatalUnkownError);
        }
    }

  if (! (RunOpt_No_Last_New_Line & run_option))
    {
      br();
    }
}


static void
chsrc_view_file (const char *path)
{
  char *cmd = NULL;
  path = xy_uniform_path (path);
  if (xy_on_windows)
    {
      cmd = xy_2strjoin ("type ", path);
    }
  else
    {
      cmd = xy_2strjoin ("cat ", path);
    }
  chsrc_run (cmd, RunOpt_Dont_Notify_On_Success|RunOpt_No_Last_New_Line);
}

static void
chsrc_ensure_dir (const char *dir)
{
  dir = xy_uniform_path (dir);

  if (xy_dir_exist (dir))
    {
      return;
    }

  // 不存在就生成
  char *mkdir_cmd = NULL;
  if (xy_on_windows)
    {
      mkdir_cmd = "md ";  // 已存在时返回 errorlevel = 1
    }
  else
    {
      mkdir_cmd = "mkdir -p ";
    }
  char *cmd = xy_2strjoin (mkdir_cmd, dir);
  cmd = xy_str_to_quietcmd (cmd);
  chsrc_run (cmd, RunOpt_No_Last_New_Line|RunOpt_Dont_Notify_On_Success);
  chsrc_note2 (xy_2strjoin ("目录不存在，已自动创建 ", dir));
}

static void
chsrc_append_to_file (const char *str, const char *file)
{
  file = xy_uniform_path (file);
  char *dir = xy_parent_dir (file);
  chsrc_ensure_dir (dir);

  char *cmd = NULL;
  if (xy_on_windows)
    {
      cmd = xy_strjoin (4, "echo ", str, " >> ", file);
    }
  else
    {
      cmd = xy_strjoin (4, "echo '", str, "' >> ", file);
    }
  chsrc_run (cmd, RunOpt_No_Last_New_Line|RunOpt_Dont_Notify_On_Success);
}

static void
chsrc_prepend_to_file (const char *str, const char *file)
{
  file = xy_uniform_path (file);
  char *dir = xy_parent_dir (file);
  chsrc_ensure_dir (dir);

  char *cmd = NULL;
  if (xy_on_windows)
    {
      xy_unimplement;
    }
  else
    {
      cmd = xy_strjoin (4, "sed -i '1i ", str, "' ", file);
    }
  chsrc_run (cmd, RunOpt_No_Last_New_Line|RunOpt_Dont_Notify_On_Success);
}

static void
chsrc_overwrite_file (const char *str, const char *file)
{
  file = xy_uniform_path (file);
  char *dir = xy_parent_dir (file);
  chsrc_ensure_dir (dir);

  char *cmd = NULL;
  if (xy_on_windows)
    {
      cmd = xy_strjoin (4, "echo ", str, " > ", file);
    }
  else
    {
      cmd = xy_strjoin (4, "echo '", str, "' > ", file);
    }
  chsrc_run (cmd, RunOpt_Default);
}

static void
chsrc_backup (const char *path)
{
  char *cmd = NULL;
  bool exist = xy_file_exist (path);

  if (!exist)
    {
      chsrc_note2 (xy_2strjoin ("文件不存在,跳过备份: ", path));
      return;
    }

  if (xy_on_bsd || xy_on_macos)
    {
      /* BSD 和 macOS 的 cp 不支持 --backup 选项 */
      cmd = xy_strjoin (5, "cp -f ", path, " ", path, ".bak");
    }
  else if (xy_on_windows)
    {
      // /Y 表示覆盖
      cmd = xy_strjoin (5, "copy /Y ", path, " ", path, ".bak" );
    }
  else
    {
      cmd = xy_strjoin (5, "cp ", path, " ", path, ".bak --backup='t'");
    }

  chsrc_run (cmd, RunOpt_No_Last_New_Line|RunOpt_Dont_Notify_On_Success);
  chsrc_note2 (xy_strjoin (3, "备份文件名为 ", path, ".bak"));
}


static char *
chsrc_get_cpuarch ()
{
  char *ret;
  bool exist;

  if (xy_on_windows)
    {
      xy_unimplement;
    }

  exist = chsrc_check_program ("arch");
  if (exist)
    {
      ret = xy_run ("arch", 0, NULL);
      return ret;
    }

  exist = chsrc_check_program ("uname");
  if (exist)
    {
      ret = xy_run ("uname -m", 0, NULL);
      return ret;
    }
  else
    {
      chsrc_error ("无法检测到CPU类型");
      exit (Exit_UserCause);
    }
}

