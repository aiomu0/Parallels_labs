#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <thread>
#include <algorithm>

using namespace std;

// Класс для параллельного вычисления сумм элементов векторов
class VectorSumCalculator {
private:
	vector<vector<int>> vectors;  // Коллекция векторов для обработки
	vector<long long> sums;       // Результаты вычисления сумм
	mutex output_mutex;           // Мьютекс для синхронизации вывода

	// Генерация случайных векторов заданного размера с элементами в указанном диапазоне
	void generateVectors(int num_vectors, int vector_size, int min_val, int max_val) {
		vectors.clear();
		vectors.resize(num_vectors);
		
		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<int> dis(min_val, max_val);
		
		// Заполнение каждого вектора случайными значениями
		for (int i = 0; i < num_vectors; ++i) {
			vectors[i].resize(vector_size);
			for (int j = 0; j < vector_size; ++j) {
				vectors[i][j] = dis(gen);
			}
		}

		sums.resize(num_vectors);
	}

	// Вычисление сумм для диапазона векторов (выполняется в отдельном потоке)
	void calculateSumsRange(int start_idx, int end_idx, int thread_id) {
		// Обработка назначенного диапазона векторов
		for (int i = start_idx; i < end_idx; ++i) {
			long long sum = 0;
			for (int value : vectors[i]) {
				sum += value;
			}
			sums[i] = sum;
		}
		
		// Защищенный вывод информации о завершении работы потока
		{ 
			lock_guard<mutex> lock(output_mutex);
			cout << "Поток " << thread_id << " обработал векторы " 
			     << start_idx << " - " << (end_idx - 1) << endl;
		}
	}

	// Измерение времени выполнения вычислений с заданным количеством потоков
	double measureExecutionTime(int num_threads) {
		vector<thread> threads;
		int vectors_per_thread = vectors.size() / num_threads;
		int remaining_vectors = vectors.size() % num_threads;
		
		// Начало отсчета времени
		auto start = chrono::high_resolution_clock::now();

		// Создание и запуск потоков с равномерным распределением нагрузки
		int current_start = 0;
		for (int i = 0; i < num_threads; ++i) {
			int vectors_for_this_thread = vectors_per_thread;
			// Распределение остатка векторов по первым потокам
			if (i < remaining_vectors) {
				++vectors_for_this_thread;
			}
			int end_idx = current_start + vectors_for_this_thread;
			threads.emplace_back(&VectorSumCalculator::calculateSumsRange, this, current_start, end_idx, i + 1);
			current_start = end_idx;
		}

		// Ожидание завершения всех потоков
		for (auto& thread : threads) {
			thread.join();
		}
		
		// Окончание отсчета времени и вычисление длительности
		auto end = chrono::high_resolution_clock::now();
		auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
		return duration.count() / 1000.0;  // Перевод в миллисекунды
	}
	
	// Проверка корректности вычислений путем выборочного пересчета сумм
	bool validateResults() {
		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<int> dis(0, vectors.size() - 1);

		// Проверка 10 случайных векторов
		for (int test = 0; test < 10; ++test) {
			int idx = dis(gen);
			long long expected_sum = 0;
			for (int value : vectors[idx]) {
				expected_sum += value;
			}
			if (sums[idx] != expected_sum) {
				cout << "Ошибка в вычислениях для вектора " << idx << endl;
				return false;
			}
		}
		return true;
	}

public:
	// Основной метод для запуска тестирования производительности
	void runBenchmark() {
		cout << "=== Тест производительности параллельного вычисления сумм векторов ===" << endl;
		cout << "Параметры: 30000 векторов по 100 элементов каждый" << endl;
		cout << "Диапазон значений: [-1000, 1000]" << endl << endl;

		// Генерация тестовых данных
		cout << "Генерация векторов..." << endl;
		generateVectors(30000, 100, -1000, 1000);
		cout << "Все 30000 векторов были сгенерированы" << endl << endl;

		vector<int> thread_counts = {1, 2, 4};
		vector<double> execution_times;
		
		// Выполнение тестов с различным количеством потоков
		for (int num_threads : thread_counts) {
			cout << "~ Тест с " << num_threads << " поток" << (num_threads > 1 ? "ами" : "ом") << " ~" << endl;
			
			fill(sums.begin(), sums.end(), 0); 
			double time = measureExecutionTime(num_threads);
			execution_times.push_back(time);
			
			// Валидация результатов
			if (validateResults()) {
				cout << "  ...Результаты корректны..." << endl;
			}
			
			cout << "  Время выполнения - " << fixed << setprecision(2) << time << " мс" << endl << endl;
		}

		// Вывод сводной таблицы результатов
		cout << "=== Полученные результаты ===" << endl;
		cout << "        Потоки      " << "Время [мс]" << endl;
			
		for (size_t i = 0; i < thread_counts.size(); ++i) {
			cout << setw(12) << thread_counts[i] << setw(15) << fixed << setprecision(2) << execution_times[i] << endl;
		}
	
		// Вывод статистики по вычисленным суммам
		cout << endl << "=== Статистика по суммам ===" << endl;
		long long min_sum = *min_element(sums.begin(), sums.end());
		long long max_sum = *max_element(sums.begin(), sums.end());

		long long total_sum = 0;
		for (long long sum : sums) {
			total_sum += sum;
		}

		double avg_sum = static_cast<double>(total_sum) / sums.size();

		cout << "   Минимальная сумма - " << min_sum << endl;
		cout << "   Максимальная сумма - " << max_sum << endl;
		cout << "   Средняя сумма - " << fixed << setprecision(2) << avg_sum << endl << endl;

		// Вывод примеров результатов
		cout << "Первые 10 сумм векторов: " << endl;
		for (int i = 0; i < 10 && i < static_cast<int>(sums.size()); ++i) {
			cout << "Вектор " << i << ": " << sums[i] << endl;
		}
	}				
};

// Точка входа в программу
int main() {
	try {
		VectorSumCalculator calculator;
		calculator.runBenchmark();
	}
	catch (const exception& e) {
		cerr << "Ошибка " << e.what() << endl;
		return 1;
	}
	
	return 0;
}