#include "day.h"
#include "eph.h"
#include "JD.h"

namespace sxtwl
{
	GZ  getShiGz(uint8_t dayTg, uint8_t hour, bool isZaoWanZiShi = true);
};

void Day::checkSSQ()
{
	if (!SSQPtr->ZQ.size() || this->d0 < SSQPtr->ZQ[0] || this->d0 >= SSQPtr->ZQ[24])
	{
		SSQPtr->calcY(this->d0);
	}
}

/**
	 * 确定已经计算过阴历信息
	 */
void Day::checkLunarData()
{
	// 已经计算过了
	if (this->Ldn != 0)
	{
		return;
	}
	this->checkSSQ();

	int mk = int2((this->d0 - SSQPtr->HS[0]) / 30);
	if (mk < 13 && SSQPtr->HS[mk + 1] <= this->d0)
	{
		mk++; //农历所在月的序数
	}

	//if (this.d0 == SSQPtr->HS[mk]) { //月的信息
	this->Lmc = SSQPtr->ym[mk];                              //月名称
	this->Ldn = SSQPtr->dx[mk];                              //月大小
	this->Lleap = (SSQPtr->leap != 0 && SSQPtr->leap == mk); //闰状况
	//}

	// 阴历所处的日
	this->Ldi = this->d0 - SSQPtr->HS[mk];
}

void Day::checkSolarData()
{
	if (this->m != 0)
	{
		return;
	}

	Time t = JD::JD2DD(this->d0 + J2000);
	this->y = t.Y;
	this->d = t.D;
	this->m = t.M;
}

/**
 * 计算节气数据
 */
void Day::checkJQData()
{
	if (this->qk != -2)
	{
		return;
	}

	this->qk = -1;
	this->getJieQiJD();

	//this->checkSSQ();

	//int qk = int2((this->d0 - SSQPtr->ZQ[0] - 7) / 15.2184);
	//////节气的取值范围是0-23
	//if (qk < 23 && this->d0 >= SSQPtr->ZQ[qk + 1])
	//{
	//    qk++;
	//}

	//this->qk = -1;
	//if (this->d0 == SSQPtr->ZQ[qk])
	//{
	//    this->qk = qk;
	//}
}

Day *Day::after(int day)
{
	return new Day(this->d0 + day);
}

Day *Day::before(int day)
{
	return new Day(this->d0 - day);
}

/**
 * 获取阴历日期
 */
int Day::getLunarDay()
{
	this->checkLunarData();
	return this->Ldi + 1;
}

/**
	 * 获取阴历月
	 */
uint8_t Day::getLunarMonth()
{
	this->checkLunarData();
	static const int yueIndex[12] = { 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	return yueIndex[this->Lmc];
}

int Day::getLunarYear(bool chineseNewYearBoundary)
{
	// 以立春为界
	if (chineseNewYearBoundary == false)
	{
		if (this->Lyear == 0)
		{
			this->checkSSQ();
			long double D = SSQPtr->ZQ[3] + (this->d0 < SSQPtr->ZQ[3] ? -365 : 0) + 365.25 * 16 - 35; //以立春为界定纪年
			this->Lyear = int2(D / 365.2422 + 0.5);
		}
		return this->Lyear + 1984;
	}
	// 以春节为界
	if (this->Lyear0 == 0)
	{
		this->checkSSQ();
		int D = SSQPtr->HS[2]; //一般第3个月为春节
		for (int j = 0; j < 14; j++)
		{ //找春节
			if (SSQPtr->ym[j] != 2 || SSQPtr->leap == j && j)
				continue;
			D = SSQPtr->HS[j];
			if (this->d0 < D)
			{
				D -= 365;
				break;
			} //无需再找下一个正月
		}
		D = D + 5810; //计算该年春节与1984年平均春节(立春附近)相差天数估计
		this->Lyear0 = int2(D / 365.2422 + 0.5);
	}
	return this->Lyear0 + 1984;
}

GZ Day::getYearGZ(bool chineseNewYearBoundary)
{
	//以春节为界
	if (chineseNewYearBoundary)
	{
		if (this->Lyear3 == NULL)
		{
			int year = this->getLunarYear(chineseNewYearBoundary) - 1984;
			int D = year + 12000;
			this->Lyear3 = new GZ(D % 10, D % 12);
		}
		return *(this->Lyear3);
	}

	// 以立春为界
	if (this->Lyear2 == NULL)
	{
		int year = this->getLunarYear(false) - 1984;
		int D = year + 12000;
		this->Lyear2 = new GZ(D % 10, D % 12);
	}
	return *(this->Lyear2);
}

GZ Day::getMonthGZ()
{
    if (this->Lmonth2 == NULL)
    {
        // Ensure solar terms for the current date's year are loaded.
        this->checkSSQ();

        // Correct mapping of BaZi Month's Earthly Branch (地支) to the starting Solar Term (节) index in ZQ.
        // ZQ array indices: 1:小寒, 3:立春, 5:惊蛰, 7:清明, 9:立夏, 11:芒种, 13:小暑, 15:立秋, 17:白露, 19:寒露, 21:立冬, 23:大雪
        const int BAZI_MONTH_MAP[12][2] = {
            {23, 0},  // 子月 (Zi, dz=0) starts at 大雪 (Daxue, ZQ[23])
            {21, 11}, // 亥月 (Hai, dz=11) starts at 立冬 (Lidong, ZQ[21])
            {19, 10}, // 戌月 (Xu, dz=10) starts at 寒露 (Hanlu, ZQ[19])
            {17, 9},  // 酉月 (You, dz=9) starts at 白露 (Bailu, ZQ[17])
            {15, 8},  // 申月 (Shen, dz=8) starts at 立秋 (Liqiu, ZQ[15])
            {13, 7},  // 未月 (Wei, dz=7) starts at 小暑 (Xiaoshu, ZQ[13])
            {11, 6},  // 午月 (Wu, dz=6) starts at 芒种 (Mangzhong, ZQ[11])
            {9, 5},   // 巳月 (Si, dz=5) starts at 立夏 (Lixia, ZQ[9])
            {7, 4},   // 辰月 (Chen, dz=4) starts at 清明 (Qingming, ZQ[7])
            {5, 3},   // 卯月 (Mao, dz=3) starts at 惊蛰 (Jingzhe, ZQ[5])
            {3, 2},   // 寅月 (Yin, dz=2) starts at 立春 (Lichun, ZQ[3])
            {1, 1}    // 丑月 (Chou, dz=1) starts at 小寒 (Xiaohan, ZQ[1])
        };

        int month_dz_index = 1; // Default to Chou month of the current year

        // Iterate through the solar terms of the current year from latest to earliest.
        for (int i = 0; i < 12; ++i) {
            int jie_qi_index = BAZI_MONTH_MAP[i][0];
            if (this->d0 >= SSQPtr->ZQ[jie_qi_index]) {
                month_dz_index = BAZI_MONTH_MAP[i][1];
                goto found_month;
            }
        }

        // If not found, the date must be in a month that started in the previous year (e.g., Jan is in last year's Chou month).
        // Load previous year's solar term data.
        SSQPtr->calcY(this->d0 - 365);
        
        // We only need to check for Daxue (start of Zi month) from the previous year.
        // If it's earlier than that, it would have been caught by the loop above (e.g. Xiaohan).
        if (this->d0 >= SSQPtr->ZQ[23]) {
            month_dz_index = 0; // Zi month
        }
        // Restore SSQ state for subsequent calculations
        SSQPtr->calcY(this->d0);

    found_month:
        // 2. 确定月柱天干 (Heavenly Stem) - 使用五虎遁
        GZ yearGZ = this->getYearGZ(false); // false 表示以立春为界
        uint8_t year_tg = yearGZ.tg;

        uint8_t start_tg_index;
        if (year_tg == 0 || year_tg == 5) start_tg_index = 2; // 甲/己 -> 丙
        else if (year_tg == 1 || year_tg == 6) start_tg_index = 4; // 乙/庚 -> 戊
        else if (year_tg == 2 || year_tg == 7) start_tg_index = 6; // 丙/辛 -> 庚
        else if (year_tg == 3 || year_tg == 8) start_tg_index = 8; // 丁/壬 -> 壬
        else start_tg_index = 0; // 戊/癸 -> 甲

        // The month order starts from Yin (寅) as the first month (index 0).
        int month_order_index = (month_dz_index - 2 + 12) % 12;
        uint8_t month_tg_index = (start_tg_index + month_order_index) % 10;

        this->Lmonth2 = new GZ(month_tg_index, month_dz_index);
    }
    return *(this->Lmonth2);
}

GZ Day::getDayGZ()
{
	if (this->Lday2 == NULL)
	{
		int D = this->d0 - 6 + 9000000;
		this->Lday2 = new GZ(D % 10, D % 12);
	}
	return *(this->Lday2);
}

GZ Day::getHourGZ(uint8_t hour, bool isZaoWanZiShi)
{
	GZ dayGZ = this->getDayGZ();
	return sxtwl::getShiGz(dayGZ.tg, hour, isZaoWanZiShi);
}

bool Day::isLunarLeap()
{
	this->checkLunarData();
	return this->Lleap;
}

int Day::getSolarYear()
{
	this->checkSolarData();
	return this->y;
}

uint8_t Day::getSolarMonth()
{
	this->checkSolarData();
	return this->m;
}

int Day::getSolarDay()
{
	this->checkSolarData();
	return this->d;
}

uint8_t Day::getWeek()
{
	if (this->week == 0xFF)
	{
		this->week = (this->d0 + J2000 + 1 + 7000000) % 7;
	}
	return this->week;
}

// 处于该月的第几周
uint8_t Day::getWeekIndex()
{
	int i = (this->getSolarDay() - 1) % 7;

	int w0 = 0;
	if (this->getWeek() >= i)
	{
		w0 = this->getWeek() - i;
	}
	else
	{
		w0 = this->getWeek() + 7 - i;
	}
	return int2((w0 + this->getSolarDay() - 1) / 7) + 1;
}
//是否有节气
bool Day::hasJieQi()
{
	this->checkJQData();
	return this->qk != -1;
}
// 获取节气
uint8_t Day::getJieQi()
{
	this->checkJQData();
	return this->qk;
}



double Day::getJieQiJD()
{
	if (this->jqjd != 0)
	{
		return this->jqjd;
	}

	long double d, xn, jd2 = this->d0 + dt_T(this->d0) - (long double)8 / (long double)24;
	long double w = XL::S_aLon(jd2 / 36525, 3);
	w = int2((w - 0.13) / pi2 * 24) * pi2 / 24;
	int D = 0;

	do
	{
		d = qi_accurate(w);
		D = int2(d + 0.5);
		// 计算出的节令值
		xn = int2(w / pi2 * 24 + 24000006.01) % 24;
		w += pi2 / 24;
		if (D > this->d0)
			break;
		if (D < this->d0)
			continue;
		if (D == this->d0)
		{
			Time t1 = JD::JD2DD(d);
			Time t2 = JD::JD2DD(D + J2000);

			t2.h = t1.h;
			t2.m = t1.m;
			t2.s = t1.s;

			auto jd = JD::toJD(t2);

			this->jqjd = jd;
			this->qk = xn;
			break;
		}
	} while (D + 12 < this->d0);

	return this->jqjd; //+ J2000;
}

// 获取星座
uint8_t Day::getConstellation()
{
	if (this->XiZ == 0xFF)
	{
		this->checkSSQ();
		int mk = int2((this->d0 - SSQPtr->ZQ[0] - 15) / 30.43685);
		//星座所在月的序数,(如果j=13,ob.d0不会超过第14号中气)
		if (mk < 11 && this->d0 >= SSQPtr->ZQ[2 * mk + 2])
		{
			mk++;
		}
		this->XiZ = (mk + 12) % 12;
	}
	return this->XiZ;
}
