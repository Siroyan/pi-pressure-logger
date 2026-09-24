#pragma once
#include <time.h>
// 待機せずに現在日時を一度だけ読む。有効な日時を取得できなければfalse。
bool readRtcTime(struct tm& value);
