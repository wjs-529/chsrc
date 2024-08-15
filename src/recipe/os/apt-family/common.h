/** ------------------------------------------------------------
 * SPDX-License-Identifier: GPL-3.0-or-later
 * -------------------------------------------------------------
 * File Authors  : Aoran Zeng <ccmywish@qq.com>
 * Contributors  :  Nil Null  <nil@null.org>
 * Created On    : <2024-06-14>
 * Last Modified : <2024-08-16>
 * ------------------------------------------------------------*/

#define OS_Apt_SourceList   "/etc/apt/sources.list"
#define OS_Apt_SourceList_D "/etc/apt/sources.list.d/"

/**
 * @note 从 Debian 12 开始，Debain 的软件源配置文件变更为 DEB822 格式，
 *       路径为:  /etc/apt/sources.list.d/debian.sources"
 *
 * @note 从 Ubuntu 24.04 开始，Ubuntu 的软件源配置文件变更为 DEB822 格式，
 *       路径为:  /etc/apt/sources.list.d/ubuntu.sources
 */
#define OS_Debian_SourceList_DEB822 "/etc/apt/sources.list.d/debian.sources"
#define OS_Ubuntu_SourceList_DEB822 "/etc/apt/sources.list.d/ubuntu.sources"


#define ETC_os_release    "/etc/os-release"

#define OS_Is_Debian_Literally  1
#define OS_Is_Ubuntu            2

// independent
#define OS_ROS_SourceList         OS_Apt_SourceList_D "ros-latest.list"

// Ubuntu based
#define OS_LinuxMint_SourceList   OS_Apt_SourceList_D "official-package-repositories.list"

// Debian based
#define OS_Armbian_SourceList     OS_Apt_SourceList_D "armbian.list"
#define OS_RaspberryPi_SourceList OS_Apt_SourceList_D "raspi.list"


/**
 * 当不存在该文件时，我们只能拼凑一个假的出来，但该函数目前只适用于 Ubuntu 和 Debian
 * 因为其它的 Debian 变体可能不使用 OS_Apt_SourceList，也可能并不适用 `VERSION_CODENAME`
 *
 * @return 文件是否存在
 */
bool
ensure_apt_sourcelist (int debian_type)
{
  bool exist = chsrc_check_file (OS_Apt_SourceList);

  if (exist)
    {
      return true;
    }
  else
    {
      chsrc_note2 ("将生成新的源配置文件");
    }

  // 反向引用需要escape一下
  char *codename = xy_run ("sed -nr 's/VERSION_CODENAME=(.*)/\\1/p' " ETC_os_release, 0, NULL);
  codename = xy_str_delete_suffix (codename, "\n");

  char *version_id = xy_run ("sed -nr 's/VERSION_ID=(.*)/\\1/p' " ETC_os_release, 0, NULL);
  version_id = xy_str_delete_suffix (codename, "\n");
  double version = atof (version_id);

  char *makeup = NULL;

  if (debian_type == OS_Is_Ubuntu)
    {
      makeup = xy_strjoin (9,
               "# Generated by chsrc " Chsrc_Version "\n\n"
               "deb " Chsrc_Maintain_URL "/ubuntu ", codename, " main restricted universe multiverse\n"
               "deb " Chsrc_Maintain_URL "/ubuntu ", codename, "-updates main restricted universe multiverse\n"
               "deb " Chsrc_Maintain_URL "/ubuntu ", codename, "-backports main restricted universe multiverse\n"
               "deb " Chsrc_Maintain_URL "/ubuntu ", codename, "-security main restricted universe multiverse\n");
    }
  else
    {
      if (version >= 12)
        {
          // https://wiki.debian.org/SourcesList
          // https://mirrors.tuna.tsinghua.edu.cn/help/debian/
          // 从 Debian 12 开始，开始有一项 non-free-firmware
          makeup = xy_strjoin (9,
               "# Generated by chsrc " Chsrc_Version "\n\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, " main contrib non-free non-free-firmware\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, "-updates main contrib non-free non-free-firmware\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, "-backports main contrib non-free non-free-firmware\n"
               "deb " Chsrc_Maintain_URL "/debian-security ", codename, "-security main contrib non-free non-free-firmware\n");
               // 上述 debian-security 这种写法是和 Debian 10不同的，所以我们只能支持 Debian 11+
        }
      else if (version >= 11)
        {
          makeup = xy_strjoin (9,
               "# Generated by chsrc " Chsrc_Version "(" Chsrc_Maintain_URL ")\n\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, " main contrib non-free\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, "-updates main contrib non-free\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, "-backports main contrib non-free\n"
               "deb " Chsrc_Maintain_URL "/debian-security ", codename, "-security main contrib non-free\n");
        }
      else if (version >= 10)
        {
          makeup = xy_strjoin (9,
               "# Generated by chsrc " Chsrc_Version "(" Chsrc_Maintain_URL ")\n\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, " main contrib non-free\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, "-updates main contrib non-free\n"
               "deb " Chsrc_Maintain_URL "/debian ", codename, "-backports main contrib non-free\n"
               "deb " Chsrc_Maintain_URL "/debian-security ", codename, "/updates main contrib non-free\n");
               // 上述 debian-security 这种写法是和 Debian 11 不同的
        }
      else
        {
          chsrc_error ("您的Debian版本过低(<10)，暂不支持换源");
          exit (Exit_Unsupported);
        }
    }

  FILE *f = fopen (OS_Apt_SourceList, "w");
  fwrite (makeup, strlen (makeup), 1, f);
  fclose (f);
  return false;
}
