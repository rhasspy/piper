.PHONY: clean docker test test-python test-cpp test-all coverage

all:
	cmake -Bbuild -DCMAKE_INSTALL_PREFIX=install
	cmake --build build --config Release
	cd build && ctest --config Release
	cmake --install build

docker:
	docker buildx build . --platform linux/amd64,linux/arm64,linux/arm/v7 --output 'type=local,dest=dist'

# Run all tests
test: test-all

# Run all tests (Python and C++)
test-all:
	@echo "Running all tests..."
	python3 run_tests.py

# Run only Python tests
test-python:
	@echo "Running Python tests..."
	python3 run_tests.py --python

# Run only C++ tests  
test-cpp:
	@echo "Running C++ tests..."
	python3 run_tests.py --cpp

# Run tests with coverage
coverage:
	@echo "Running tests with coverage..."
	python3 run_tests.py --coverage

# Run only unit tests
test-unit:
	@echo "Running unit tests..."
	python3 run_tests.py --unit

# Run only integration tests
test-integration:
	@echo "Running integration tests..."
	python3 run_tests.py --integration

# Run only Japanese TTS tests
test-japanese:
	@echo "Running Japanese TTS tests..."
	python3 run_tests.py --japanese

# Install test dependencies
install-test-deps:
	pip install -r src/python/requirements_test.txt

clean:
	rm -rf build install dist htmlcov .coverage coverage.xml .pytest_cache
	find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
	find . -type f -name "*.pyc" -delete 2>/dev/null || true
