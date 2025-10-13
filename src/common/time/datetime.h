/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2010
//

#pragma once

#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <iomanip>
#include <iostream>

#include "common/defs.h"
#include "common/lang/string.h"

namespace common {

/*
 *  \brief Date and time are represented as integer for ease of
 *   calculation and comparison.
 *
 *  Julian day number is the integer number of days that have elapsed since
 *  the defined as noon Universal Time (UT) Monday, January 1, 4713 BC.
 *
 *  Date and Time stored as a Julian day number and number of
 *  milliseconds since midnight.  Does not perform any timezone
 *  calculations.  All magic numbers and related calculations
 *  have been taken from:
 *
 *  \sa http://www.faqs.org/faqs/calendars.faq
 *  \sa http://scienceworld.wolfram.com/astronomy/JulianDate.html
 *  \sa http://scienceworld.wolfram.com/astronomy/GregorianCalendar.html
 *  \sa http://scienceworld.wolfram.com/astronomy/Weekday.html
 */  

/**
 * @brief 日期时间处理模块
 * @defgroup Time 时间处理相关功能
 * @details 提供日期时间的表示、转换和操作功能，包括日期时间结构体、时间戳、时间和日期类等
 */

/**
 * @brief 日期时间结构体，使用整数表示日期和时间，便于计算和比较
 * @ingroup Time
 * 
 * 日期使用儒略日（Julian day number）表示，即从公元前4713年1月1日中午开始经过的天数。
 * 时间使用自午夜以来的毫秒数表示。
 * 此实现不处理时区转换，所有日期时间均以UTC时间为准。
 */
struct DateTime
{
  int m_date;  ///< 儒略日，整数日期表示
  int m_time;  ///< 自午夜以来的毫秒数

  /**
   * @brief 时间单位常量定义
   */
  enum
  {
    SECONDS_PER_DAY  = 86400,    ///< 一天的秒数
    SECONDS_PER_HOUR = 3600,     ///< 一小时的秒数
    SECONDS_PER_MIN  = 60,       ///< 一分钟的秒数
    MINUTES_PER_HOUR = 60,       ///< 一小时的分钟数

    MILLIS_PER_DAY  = 86400000,  ///< 一天的毫秒数
    MILLIS_PER_HOUR = 3600000,   ///< 一小时的毫秒数
    MILLIS_PER_MIN  = 60000,     ///< 一分钟的毫秒数
    MILLIS_PER_SEC  = 1000,      ///< 一秒的毫秒数

    // time_t epoch (1970-01-01) as a Julian date
    JULIAN_19700101 = 2440588    ///< 1970-01-01对应的儒略日
  };
  
  /**
   * @brief 月份常量定义
   */
  enum
  {
    MON_JAN = 1,   ///< 一月
    MON_FEB = 2,   ///< 二月
    MON_MAR = 3,   ///< 三月
    MON_APR = 4,   ///< 四月
    MON_MAY = 5,   ///< 五月
    MON_JUN = 6,   ///< 六月
    MON_JUL = 7,   ///< 七月
    MON_AUG = 8,   ///< 八月
    MON_SEP = 9,   ///< 九月
    MON_OCT = 10,  ///< 十月
    MON_NOV = 11,  ///< 十一月
    MON_DEC = 12   ///< 十二月
  };

  /**
   * @brief 默认构造函数，初始化为零
   */
  DateTime() : m_date(0), m_time(0) {}

  /**
   * @brief 从儒略日和毫秒时间构造
   * @param[in] date 儒略日
   * @param[in] time 自午夜以来的毫秒数
   */
  DateTime(int date, int time) : m_date(date), m_time(time) {}

  /**
   * @brief 从指定的日期时间组件构造
   * @param[in] year 年份
   * @param[in] month 月份
   * @param[in] day 日期
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] millis 毫秒
   */
  DateTime(int year, int month, int day, int hour, int minute, int second, int millis)
  {
    m_date = julian_date(year, month, day);
    m_time = make_hms(hour, minute, second, millis);
  }

  /**
   * @brief 从XML日期时间格式构造
   * @param[in] xml_time XML格式的日期时间字符串
   */
  DateTime(string &xml_time);

  /**
   * @brief 检查字符串是否为有效的XML日期时间格式
   * @param[in] str 待检查的字符串
   * @return 有效返回true，否则返回false
   */
  static bool is_valid_xml_datetime(const string &str);

  /**
   * @brief 获取年月日信息
   * @param[out] year 年份
   * @param[out] month 月份
   * @param[out] day 日期
   */
  inline void get_ymd(int &year, int &month, int &day) const { get_ymd(m_date, year, month, day); }

  /**
   * @brief 获取时分秒毫秒信息
   * @param[out] hour 小时
   * @param[out] minute 分钟
   * @param[out] second 秒
   * @param[out] millis 毫秒
   */
  inline void get_hms(int &hour, int &minute, int &second, int &millis) const
  {
    int ticks = m_time / MILLIS_PER_SEC;
    hour      = ticks / SECONDS_PER_HOUR;
    minute    = (ticks / SECONDS_PER_MIN) % MINUTES_PER_HOUR;
    second    = ticks % SECONDS_PER_MIN;
    millis    = m_time % MILLIS_PER_SEC;
  }

  /**
   * @brief 转换为time_t类型
   * @note 在32位平台上，超过2038年后可能会溢出
   * @return 对应的time_t值
   */
  inline time_t to_time_t() const { return (SECONDS_PER_DAY * (m_date - JULIAN_19700101) + m_time / MILLIS_PER_SEC); }

  /**
   * @brief 转换为struct tm结构体（UTC时间）
   * @return 对应的tm结构体
   */
  tm to_tm() const
  {
    int year, month, day;
    int hour, minute, second, millis;
    tm  result = {0};

    get_ymd(year, month, day);
    get_hms(hour, minute, second, millis);

    result.tm_year  = year - 1900;
    result.tm_mon   = month - 1;
    result.tm_mday  = day;
    result.tm_hour  = hour;
    result.tm_min   = minute;
    result.tm_sec   = second;
    result.tm_isdst = -1;

    return result;
  }

  /**
   * @brief 设置日期部分
   * @param[in] year 年份
   * @param[in] month 月份
   * @param[in] day 日期
   */
  void set_ymd(int year, int month, int day) { m_date = julian_date(year, month, day); }

  /**
   * @brief 设置时间部分
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] millis 毫秒
   */
  void set_hms(int hour, int minute, int second, int millis) { m_time = make_hms(hour, minute, second, millis); }

  /**
   * @brief 清除日期部分
   */
  void clear_date() { m_date = 0; }

  /**
   * @brief 清除时间部分
   */
  void clear_time() { m_time = 0; }

  /**
   * @brief 设置内部日期和时间成员
   * @param[in] date 儒略日
   * @param[in] time 自午夜以来的毫秒数
   */
  void set(int date, int time)
  {
    m_date = date;
    m_time = time;
  }

  /**
   * @brief 从另一个DateTime初始化
   * @param[in] other 另一个DateTime对象
   */
  void set(const DateTime &other)
  {
    m_date = other.m_date;
    m_time = other.m_time;
  }

  /**
   * @brief 增加指定秒数
   * @param[in] seconds 要增加的秒数
   */
  void operator+=(int seconds)
  {
    int d = seconds / SECONDS_PER_DAY;
    int s = seconds % SECONDS_PER_DAY;

    m_date += d;
    m_time += s * MILLIS_PER_SEC;

    if (m_time > MILLIS_PER_DAY) {
      m_date++;
      m_time %= MILLIS_PER_DAY;
    } else if (m_time < 0) {
      m_date--;
      m_time += MILLIS_PER_DAY;
    }
  }

  /**
   * @brief 转换为XML Schema日期时间格式的字符串
   * @return XML格式的日期时间字符串
   */
  string to_xml_date_time();

  /**
   * @brief 从XML格式的日期时间字符串转换为time_t
   * @param[in] xml_str XML格式的日期时间字符串
   * @return 对应的time_t值
   */
  time_t str_to_time_t(string &xml_str);

  /**
   * @brief 从time_t转换为XML格式的日期时间字符串
   * @param[in] timet time_t值
   * @return XML格式的日期时间字符串
   */
  string time_t_to_xml_str(time_t timet);

  /**
   * @brief 从time_t转换为字符串表示
   * @param[in] timet time_t值
   * @return 字符串表示
   */
  string time_t_to_str(int timet);

  /**
   * @brief 从XML格式的日期时间字符串转换为time_t的字符串表示
   * @param[in] xml_str XML格式的日期时间字符串
   * @return time_t的字符串表示
   */
  string str_to_time_t_str(string &xml_str);

  /**
   * @brief 辅助方法：将时分秒毫秒转换为自午夜以来的毫秒数
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] millis 毫秒
   * @return 自午夜以来的毫秒数
   */
  static int make_hms(int hour, int minute, int second, int millis)
  {
    return MILLIS_PER_SEC * (SECONDS_PER_HOUR * hour + SECONDS_PER_MIN * minute + second) + millis;
  }

  /**
   * @brief 获取当前系统时间
   * @return 当前时间的DateTime对象
   */
  static DateTime now();

  /**
   * @brief 获取当前系统时间的time_t表示
   * @return 当前时间的time_t值
   */
  time_t nowtimet();

  /**
   * @brief 从time_t和可选的毫秒数转换为DateTime
   * @param[in] t time_t值
   * @param[in] millis 毫秒数，默认为0
   * @return 对应的DateTime对象
   */
  static DateTime from_time_t(time_t t, int millis = 0)
  {
    struct tm tmbuf;
    tm       *tm = gmtime_r(&t, &tmbuf);
    return from_tm(*tm, millis);
  }

  /**
   * @brief 从tm结构体和可选的毫秒数转换为DateTime
   * @note tm结构体假定包含UTC时间
   * @param[in] tm tm结构体
   * @param[in] millis 毫秒数，默认为0
   * @return 对应的DateTime对象
   */
  static DateTime from_tm(const tm &tm, int millis = 0)
  {
    return DateTime(
        julian_date(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday), make_hms(tm.tm_hour, tm.tm_min, tm.tm_sec, millis));
  }

  /**
   * @brief 辅助方法：计算儒略日
   * @param[in] year 年份
   * @param[in] month 月份
   * @param[in] day 日期
   * @return 对应的儒略日
   */
  static int julian_date(int year, int month, int day)
  {
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    return (day + int((153 * m + 2) / 5) + y * 365 + int(y / 4) - int(y / 100) + int(y / 400) - 32045);
  }

  /**
   * @brief 将儒略日转换为年月日
   * @param[in] jday 儒略日
   * @param[out] year 年份
   * @param[out] month 月份
   * @param[out] day 日期
   */
  static void get_ymd(int jday, int &year, int &month, int &day)
  {
    int a = jday + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - int((b * 146097) / 4);
    int d = (4 * c + 3) / 1461;
    int e = c - int((1461 * d) / 4);
    int m = (5 * e + 2) / 153;
    day   = e - int((153 * m + 2) / 5) + 1;
    month = m + 3 - 12 * int(m / 10);
    year  = b * 100 + d - 4800 + int(m / 10);
  }

  /**
   * @brief 返回本地时区的人类可读字符串表示
   * @return 本地时区的日期时间字符串
   */
  string to_string_local()
  {
    const time_t tt = to_time_t();
    // 'man asctime' specifies that buffer must be at least 26 bytes
    char      buffer[32];
    struct tm tm;
    asctime_r(localtime_r(&tt, &tm), &(buffer[0]));
    string s(buffer);
    return s;
  }

  /**
   * @brief 返回UTC时区的人类可读字符串表示
   * @return UTC时区的日期时间字符串
   */
  string to_string_utc()
  {
    const time_t tt = to_time_t();
    // 'man asctime' specifies that buffer must be at least 26 bytes
    char      buffer[32];
    struct tm tm;
    asctime_r(gmtime_r(&tt, &tm), &(buffer[0]));
    string s(buffer);
    return s;
  }

  /**
   * @brief 增加指定的持续时间并返回结果的time_t表示
   * @param[in] xml_dur XML格式的持续时间字符串
   * @return 增加持续时间后的time_t值
   */
  time_t add_duration(string xml_dur);

  /**
   * @brief 增加指定的持续时间
   * @param[in] xml_dur XML格式的持续时间字符串
   */
  void add_duration_date_time(string xml_dur);

  /**
   * @brief 获取指定年月的最大天数
   * @param[in] year 年份
   * @param[in] month 月份
   * @return 该月的最大天数
   */
  int max_day_in_month_for(int year, int month);

  /**
   * @brief 解析持续时间字符串并转换为struct tm
   * @param[in] dur_str 持续时间字符串
   * @param[out] tm_t 用于存储解析结果的tm结构体
   */
  void parse_duration(string dur_str, struct tm &tm_t);
};

/**
 * @brief 比较两个DateTime是否相等
 * @param[in] lhs 左侧操作数
 * @param[in] rhs 右侧操作数
 * @return 相等返回true，否则返回false
 */
inline bool operator==(const DateTime &lhs, const DateTime &rhs)
{
  return lhs.m_date == rhs.m_date && lhs.m_time == rhs.m_time;
}

/**
 * @brief 比较两个DateTime是否不相等
 * @param[in] lhs 左侧操作数
 * @param[in] rhs 右侧操作数
 * @return 不相等返回true，否则返回false
 */
inline bool operator!=(const DateTime &lhs, const DateTime &rhs) { return !(lhs == rhs); }

/**
 * @brief 比较左侧DateTime是否小于右侧DateTime
 * @param[in] lhs 左侧操作数
 * @param[in] rhs 右侧操作数
 * @return 左侧小于右侧返回true，否则返回false
 */
inline bool operator<(const DateTime &lhs, const DateTime &rhs)
{
  if (lhs.m_date < rhs.m_date)
    return true;
  else if (lhs.m_date > rhs.m_date)
    return false;
  else if (lhs.m_time < rhs.m_time)
    return true;
  return false;
}

/**
 * @brief 比较左侧DateTime是否大于右侧DateTime
 * @param[in] lhs 左侧操作数
 * @param[in] rhs 右侧操作数
 * @return 左侧大于右侧返回true，否则返回false
 */
inline bool operator>(const DateTime &lhs, const DateTime &rhs) { return !(lhs == rhs || lhs < rhs); }

/**
 * @brief 比较左侧DateTime是否小于等于右侧DateTime
 * @param[in] lhs 左侧操作数
 * @param[in] rhs 右侧操作数
 * @return 左侧小于等于右侧返回true，否则返回false
 */
inline bool operator<=(const DateTime &lhs, const DateTime &rhs) { return lhs == rhs || lhs < rhs; }

/**
 * @brief 比较左侧DateTime是否大于等于右侧DateTime
 * @param[in] lhs 左侧操作数
 * @param[in] rhs 右侧操作数
 * @return 左侧大于等于右侧返回true，否则返回false
 */
inline bool operator>=(const DateTime &lhs, const DateTime &rhs) { return lhs == rhs || lhs > rhs; }

/**
 * @brief 计算两个DateTime之间的差值（秒）
 * @param[in] lhs 左侧操作数（被减数）
 * @param[in] rhs 右侧操作数（减数）
 * @return 两个时间的差值（秒）
 */
inline int operator-(const DateTime &lhs, const DateTime &rhs)
{
  return (DateTime::SECONDS_PER_DAY * (lhs.m_date - rhs.m_date) +
          // Truncate the millis before subtracting
          lhs.m_time / 1000 - rhs.m_time / 1000);
}

/**
 * @brief UTC时区的时间戳类
 * @ingroup Time
 * @details 继承自DateTime，提供更多便捷的构造方法，默认使用当前日期时间
 */
class TimeStamp : public DateTime
{
public:
  /**
   * @brief 默认构造函数，初始化为当前日期时间
   */
  TimeStamp() : DateTime(DateTime::now()) {}

  /**
   * @brief 使用指定的时间和当前日期构造
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] millisecond 毫秒，默认为0
   */
  TimeStamp(int hour, int minute, int second, int millisecond = 0) : DateTime(DateTime::now())
  {
    set_hms(hour, minute, second, millisecond);
  }

  /**
   * @brief 使用指定的日期时间组件构造
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] date 日期
   * @param[in] month 月份
   * @param[in] year 年份
   */
  TimeStamp(int hour, int minute, int second, int date, int month, int year)
      : DateTime(year, month, date, hour, minute, second, 0)
  {}

  /**
   * @brief 使用完整的日期时间组件构造
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] millisecond 毫秒
   * @param[in] date 日期
   * @param[in] month 月份
   * @param[in] year 年份
   */
  TimeStamp(int hour, int minute, int second, int millisecond, int date, int month, int year)
      : DateTime(year, month, date, hour, minute, second, millisecond)
  {}

  /**
   * @brief 从time_t构造
   * @param[in] time time_t值
   * @param[in] millisecond 毫秒，默认为0
   */
  TimeStamp(time_t time, int millisecond = 0) : DateTime(from_time_t(time, millisecond)) {}

  /**
   * @brief 从tm结构体构造
   * @param[in] time tm结构体指针
   * @param[in] millisecond 毫秒，默认为0
   */
  TimeStamp(const tm *time, int millisecond = 0) : DateTime(from_tm(*time, millisecond)) {}

  /**
   * @brief 设置为当前时间
   */
  void set_current() { set(DateTime::now()); }
};

/**
 * @brief UTC时区的时间类（仅包含时间部分）
 * @ingroup Time
 * @details 继承自DateTime，自动清除日期部分，专注于时间处理
 */
class Time : public DateTime
{
public:
  /**
   * @brief 默认构造函数，初始化为当前时间
   */
  Time() { set_current(); }

  /**
   * @brief 从DateTime构造
   * @param[in] val 源DateTime对象
   */
  Time(const DateTime &val) : DateTime(val) { clear_date(); }

  /**
   * @brief 使用指定的时间组件构造
   * @param[in] hour 小时
   * @param[in] minute 分钟
   * @param[in] second 秒
   * @param[in] millisecond 毫秒，默认为0
   */
  Time(int hour, int minute, int second, int millisecond = 0) { set_hms(hour, minute, second, millisecond); }

  /**
   * @brief 从time_t构造
   * @param[in] time time_t值
   * @param[in] millisecond 毫秒，默认为0
   */
  Time(time_t time, int millisecond = 0) : DateTime(from_time_t(time, millisecond)) { clear_date(); }

  /**
   * @brief 从tm结构体构造
   * @param[in] time tm结构体指针
   * @param[in] millisecond 毫秒，默认为0
   */
  Time(const tm *time, int millisecond = 0) : DateTime(from_tm(*time, millisecond)) { clear_date(); }

  /**
   * @brief 设置为当前时间
   */
  void set_current()
  {
    DateTime d = now();
    m_time     = d.m_time;
  }
};

/**
 * @brief UTC时区的日期类（仅包含日期部分）
 * @ingroup Time
 * @details 继承自DateTime，自动清除时间部分，专注于日期处理
 */
class Date : public DateTime
{
public:
  /**
   * @brief 默认构造函数，初始化为当前日期
   */
  Date() { set_current(); }

  /**
   * @brief 从DateTime构造
   * @param[in] val 源DateTime对象
   */
  Date(const DateTime &val) : DateTime(val) { clear_time(); }

  /**
   * @brief 使用指定的日期组件构造
   * @param[in] date 日期
   * @param[in] month 月份
   * @param[in] year 年份
   */
  Date(int date, int month, int year) : DateTime(year, month, date, 0, 0, 0, 0) {}

  /**
   * @brief 从秒数构造（仅使用日期部分）
   * @param[in] sec 秒数
   */
  Date(long sec) : DateTime(sec / DateTime::SECONDS_PER_DAY, 0) {}

  /**
   * @brief 从tm结构体构造
   * @param[in] time tm结构体指针
   */
  Date(const tm *time) : DateTime(from_tm(*time)) { clear_time(); }

  /**
   * @brief 设置为当前日期
   */
  void set_current()
  {
    DateTime d = now();
    m_date     = d.m_date;
  }
};

/**
 * @brief 当前时间工具类
 * @ingroup Time
 * @details 提供获取当前时间的各种便捷静态方法
 */
class Now
{
public:
  /**
   * @brief 获取当前秒数（向上取整）
   * @return 当前时间的秒数
   */
  static inline int64_t sec()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    time_t sec = tv.tv_sec;
    // Round up if necessary
    if (tv.tv_usec > 500 * 1000)
      sec++;
    return sec;
  }

  /**
   * @brief 获取当前微秒数
   * @return 当前时间的微秒数
   */
  static inline int64_t usec()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
  }

  /**
   * @brief 获取当前毫秒数（向上取整）
   * @return 当前时间的毫秒数
   */
  static inline int64_t msec()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    int64_t msec = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (tv.tv_usec % 1000 >= 500)
      msec++;
    return msec;
  }

  /**
   * @brief 获取唯一的时间戳字符串
   * @return 唯一的时间戳字符串
   */
  static string unique();
};

}  // namespace common
