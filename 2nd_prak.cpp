#include <iostream>
#include <string>

using namespace std;

class Date {
protected:
	int day = 1;
	int month = 1;
	int year = 1;
	static const string names[12];
public:
	Date(int d = 1, int m = 1, int y = 1) {}
	Date(const Date& old) {}
	void show_day() const {
		cout << day << endl;
	}
	void show_month() const {
		//cout << month << endl;
		cout << names[month - 1] << endl;
	}
	void show_year() const {
		cout << year << endl;
	}
	virtual Date& operator=(const Date& old) {
		day = old.day;
		month = old.month;
		year = old.year;
		return *this;
	}
	virtual void operator[](const int& indx) = 0;
	virtual void joke() const = 0;
	virtual ~Date() {};
};
const string Date::names[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

class FunDate : public Date {
public:
	FunDate(int d = 1, int m = 1, int y = 1) {
		day = d;
		month = m;
		year = y;
	}
	FunDate(const FunDate& old) {
		day = old.day;
		month = old.month;
		year = old.year;
	}
	using Date::show_day;
	using Date::show_month;
	using Date::show_year;
	FunDate& operator=(const FunDate& old) {
		day = old.day;
		month = old.month;
		year = old.year;
		return *this;
	}
	void operator[](const int& indx) {
		if (indx < 1 || indx > 12) {
			cout << "Wrong number of month!" << endl;
			return;
		}
		cout << "The month number " << indx << " is " << names[indx - 1] << endl;
	}
	friend ostream& operator<<(ostream& os, const FunDate& d);
	friend istream& operator>>(istream& is, const FunDate& d);
	void set_date(int d, int m, int y) {
		day = d;
		month = m;
		year = y;
	}
	void joke() const {
		cout << "A very funny joke of the date " << *this << endl;
	}
	~FunDate() {}
};

ostream& operator<<(ostream& os, const FunDate& d) {
	os << d.day << " " << d.names[d.month - 1] << " " << d.year << endl;
	return os;
}

/*istream& operator>>(istream& is, const FunDate& d) {
	is >> (d.day);
	is >> (d.month);
	is >> (d.year);
	return is;
}*/

void menu() {
	cout << endl;
	cout << "Choose action to do with date:" << endl;
	cout << "0. Exit" << endl;
	cout << "1. Enter new date" << endl;
	cout << "2. Output current date" << endl;
	cout << "3. Output the joke of date" << endl;
	cout << "4. Output the day of current date" << endl;
	cout << "5. Output the month of current date" << endl;
	cout << "6. Output the year of current date" << endl;
}

void interactive_session(FunDate& date) {
	bool stop = false;
	while (!stop) {
		menu();
		int cmd;
		cin >> cmd;
		if (cmd < 0 || cmd > 6) {
			cout << "Invalid command. Try once more." << endl;
			continue;
		}
		int day, month, year;
		switch (cmd) {
		case 0:
			stop = true;
			break;
		case 1:
			cout << "Enter the day: ";
			cin >> day;
			cout << "Enter the month number: ";
			cin >> month;
			cout << "Enter the year: ";
			cin >> year;
			date.set_date(day, month, year);
			/*cout << "Example: 1 1 1" << endl;
			cin >> date;*/
			break;
		case 2:
			cout << date;
			break;
		case 3:
			date.joke();
			break;
		case 4:
			date.show_day();
			break;
		case 5:
			date.show_month();
			break;
		case 6:
			date.show_year();
			break;
		}
	}
}

int main() {
	cout << "Basic date: 1 January 1" << endl;
	FunDate date;
	interactive_session(date);
	return 0;
}