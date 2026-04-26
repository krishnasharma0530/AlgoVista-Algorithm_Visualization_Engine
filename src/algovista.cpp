#include <algorithm>
#include <cstdlib>
#include <exception>
#include <functional>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Step {
    std::string type;
    std::vector<int> array;
    int current = -1;
    int secondary = -1;
    int found = -1;
    int low = -1;
    int high = -1;
    int pivot = -1;
    std::string message;
    std::vector<std::vector<int>> treeLevels;
    std::vector<int> treeValues;
    std::vector<int> treePresent;
};

struct Stats {
    int steps = 0;
    long long comparisons = 0;
    long long swaps = 0;
};

struct RunResult {
    std::string algorithm;
    std::string category;
    std::vector<Step> steps;
    Stats stats;
    int resultIndex = -1;
    std::vector<int> finalArray;
};

struct Node {
    int data;
    int height;
    Node* left;
    Node* right;

    explicit Node(int value) : data(value), height(1), left(nullptr), right(nullptr) {}
};

std::string escapeJson(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string vectorToJson(const std::vector<int>& values) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << values[i];
    }
    out << "]";
    return out.str();
}

std::string treeLevelsToJson(const std::vector<std::vector<int>>& levels) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < levels.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << vectorToJson(levels[i]);
    }
    out << "]";
    return out.str();
}

std::vector<int> parseCsvNumbers(const std::string& csv) {
    std::vector<int> values;
    std::stringstream stream(csv);
    std::string token;

    while (std::getline(stream, token, ',')) {
        size_t first = token.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) {
            continue;
        }
        size_t last = token.find_last_not_of(" \t\n\r");
        token = token.substr(first, last - first + 1);
        values.push_back(std::stoi(token));
    }

    return values;
}

void addArrayStep(
    RunResult& result,
    const std::vector<int>& array,
    const std::string& message,
    int current = -1,
    int secondary = -1,
    int found = -1,
    int low = -1,
    int high = -1,
    int pivot = -1
) {
    Step step;
    step.type = "array";
    step.array = array;
    step.current = current;
    step.secondary = secondary;
    step.found = found;
    step.low = low;
    step.high = high;
    step.pivot = pivot;
    step.message = message;
    result.stats.steps += 1;
    result.steps.push_back(step);
}

std::vector<std::vector<int>> captureTreeLevels(Node* root) {
    std::vector<std::vector<int>> levels;
    if (!root) {
        return levels;
    }

    std::queue<Node*> nodes;
    nodes.push(root);

    while (!nodes.empty()) {
        int count = static_cast<int>(nodes.size());
        std::vector<int> level;
        level.reserve(count);

        for (int i = 0; i < count; ++i) {
            Node* current = nodes.front();
            nodes.pop();
            level.push_back(current->data);

            if (current->left) {
                nodes.push(current->left);
            }
            if (current->right) {
                nodes.push(current->right);
            }
        }

        levels.push_back(level);
    }

    return levels;
}

void captureTreeShape(Node* root, std::vector<int>& values, std::vector<int>& present) {
    values.clear();
    present.clear();

    if (!root) {
        return;
    }

    std::queue<Node*> nodes;
    nodes.push(root);

    while (!nodes.empty()) {
        int count = static_cast<int>(nodes.size());
        bool nextLevelHasRealNode = false;

        for (int i = 0; i < count; ++i) {
            Node* current = nodes.front();
            nodes.pop();

            if (current) {
                values.push_back(current->data);
                present.push_back(1);
                nodes.push(current->left);
                nodes.push(current->right);
                if (current->left || current->right) {
                    nextLevelHasRealNode = true;
                }
            } else {
                values.push_back(0);
                present.push_back(0);
                nodes.push(nullptr);
                nodes.push(nullptr);
            }
        }

        if (!nextLevelHasRealNode) {
            break;
        }
    }
}

void addTreeStep(RunResult& result, Node* root, const std::string& message) {
    Step step;
    step.type = "tree";
    step.message = message;
    step.treeLevels = captureTreeLevels(root);
    captureTreeShape(root, step.treeValues, step.treePresent);
    result.stats.steps += 1;
    result.steps.push_back(step);
}

int height(Node* node) {
    return node ? node->height : 0;
}

Node* rightRotate(Node* y) {
    Node* x = y->left;
    Node* subtree = x->right;

    x->right = y;
    y->left = subtree;

    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    return x;
}

Node* leftRotate(Node* x) {
    Node* y = x->right;
    Node* subtree = y->left;

    y->left = x;
    x->right = subtree;

    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    return y;
}

int getBalance(Node* node) {
    return node ? height(node->left) - height(node->right) : 0;
}

Node* insertAvl(Node* node, int key, RunResult& result) {
    if (!node) {
        return new Node(key);
    }

    if (key < node->data) {
        result.stats.comparisons += 1;
        node->left = insertAvl(node->left, key, result);
    } else {
        result.stats.comparisons += 1;
        node->right = insertAvl(node->right, key, result);
    }

    node->height = std::max(height(node->left), height(node->right)) + 1;
    int balance = getBalance(node);

    if (balance > 1 && key < node->left->data) {
        return rightRotate(node);
    }
    if (balance < -1 && key > node->right->data) {
        return leftRotate(node);
    }
    if (balance > 1 && key > node->left->data) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && key < node->right->data) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void destroyTree(Node* root) {
    if (!root) {
        return;
    }
    destroyTree(root->left);
    destroyTree(root->right);
    delete root;
}

void bubbleSort(std::vector<int>& arr, RunResult& result) {
    for (size_t i = 0; i < arr.size(); ++i) {
        for (size_t j = 0; j + 1 < arr.size() - i; ++j) {
            result.stats.comparisons += 1;
            addArrayStep(result, arr, "Comparing adjacent bars.", static_cast<int>(j), static_cast<int>(j + 1));
            if (arr[j] > arr[j + 1]) {
                std::swap(arr[j], arr[j + 1]);
                result.stats.swaps += 1;
                addArrayStep(result, arr, "Swap completed.", static_cast<int>(j), static_cast<int>(j + 1));
            }
        }
    }
}

void selectionSort(std::vector<int>& arr, RunResult& result) {
    for (size_t i = 0; i < arr.size(); ++i) {
        size_t minIndex = i;
        addArrayStep(result, arr, "Starting a new pass. Current minimum is highlighted.", static_cast<int>(minIndex));
        for (size_t j = i + 1; j < arr.size(); ++j) {
            result.stats.comparisons += 1;
            addArrayStep(result, arr, "Comparing current value with the current minimum.", static_cast<int>(j), static_cast<int>(minIndex));
            if (arr[j] < arr[minIndex]) {
                minIndex = j;
                addArrayStep(result, arr, "Found a new minimum value for this pass.", static_cast<int>(minIndex));
            }
        }
        if (minIndex != i) {
            std::swap(arr[i], arr[minIndex]);
            result.stats.swaps += 1;
            addArrayStep(result, arr, "Placed the minimum value into its correct position.", static_cast<int>(i), static_cast<int>(minIndex));
        }
    }
}

void insertionSort(std::vector<int>& arr, RunResult& result) {
    for (size_t i = 1; i < arr.size(); ++i) {
        int key = arr[i];
        int j = static_cast<int>(i) - 1;
        addArrayStep(result, arr, "Taking the next value and inserting it into the sorted left side.", static_cast<int>(i));

        while (j >= 0) {
            result.stats.comparisons += 1;
            addArrayStep(result, arr, "Comparing the key with the previous value.", j, static_cast<int>(i));
            if (arr[j] <= key) {
                break;
            }

            arr[j + 1] = arr[j];
            result.stats.swaps += 1;
            addArrayStep(result, arr, "Shifted a larger value one position to the right.", j, j + 1);
            j -= 1;
        }

        arr[j + 1] = key;
        addArrayStep(result, arr, "Inserted the key into its correct position.", j + 1);
    }
}

void merge(std::vector<int>& arr, int left, int middle, int right, RunResult& result) {
    std::vector<int> temp;
    temp.reserve(static_cast<size_t>(right - left + 1));

    int i = left;
    int j = middle + 1;

    while (i <= middle && j <= right) {
        result.stats.comparisons += 1;
        addArrayStep(result, arr, "Comparing values from the left and right halves.", i, j);
        if (arr[i] <= arr[j]) {
            temp.push_back(arr[i]);
            i += 1;
        } else {
            temp.push_back(arr[j]);
            j += 1;
        }
    }

    while (i <= middle) {
        temp.push_back(arr[i]);
        i += 1;
    }

    while (j <= right) {
        temp.push_back(arr[j]);
        j += 1;
    }

    for (int k = left; k <= right; ++k) {
        arr[k] = temp[static_cast<size_t>(k - left)];
        result.stats.swaps += 1;
        addArrayStep(result, arr, "Copied the merged value back into the main array.", k);
    }
}

void mergeSort(std::vector<int>& arr, int left, int right, RunResult& result) {
    if (left >= right) {
        return;
    }
    int middle = left + (right - left) / 2;
    mergeSort(arr, left, middle, result);
    mergeSort(arr, middle + 1, right, result);
    merge(arr, left, middle, right, result);
}

int partition(std::vector<int>& arr, int low, int high, RunResult& result) {
    int pivotValue = arr[high];
    int i = low - 1;
        addArrayStep(result, arr, "Using the last element as the pivot.", high, -1, -1, low, high, high);

    for (int j = low; j < high; ++j) {
        result.stats.comparisons += 1;
        addArrayStep(result, arr, "Comparing the current value with the pivot.", j, high, -1, low, high, high);
        if (arr[j] < pivotValue) {
            i += 1;
            std::swap(arr[i], arr[j]);
            result.stats.swaps += 1;
            addArrayStep(result, arr, "Moved a smaller value to the left side of the pivot.", i, j, -1, low, high, high);
        }
    }

    std::swap(arr[i + 1], arr[high]);
    result.stats.swaps += 1;
    addArrayStep(result, arr, "Placed the pivot into its final position.", i + 1, high, -1, low, high, i + 1);
    return i + 1;
}

void quickSort(std::vector<int>& arr, int low, int high, RunResult& result) {
    if (low < high) {
        int pivotIndex = partition(arr, low, high, result);
        quickSort(arr, low, pivotIndex - 1, result);
        quickSort(arr, pivotIndex + 1, high, result);
    }
}

void heapify(std::vector<int>& arr, int size, int root, RunResult& result) {
    int largest = root;
    int left = 2 * root + 1;
    int right = 2 * root + 2;

    if (left < size) {
        result.stats.comparisons += 1;
        addArrayStep(result, arr, "Comparing parent with left child.", root, left, -1, 0, size - 1);
        if (arr[left] > arr[largest]) {
            largest = left;
        }
    }

    if (right < size) {
        result.stats.comparisons += 1;
        addArrayStep(result, arr, "Comparing current largest with right child.", largest, right, -1, 0, size - 1);
        if (arr[right] > arr[largest]) {
            largest = right;
        }
    }

    if (largest != root) {
        std::swap(arr[root], arr[largest]);
        result.stats.swaps += 1;
        addArrayStep(result, arr, "Heap property restored with a swap.", root, largest, -1, 0, size - 1);
        heapify(arr, size, largest, result);
    }
}

void heapSort(std::vector<int>& arr, RunResult& result) {
    int size = static_cast<int>(arr.size());

    for (int i = size / 2 - 1; i >= 0; --i) {
        heapify(arr, size, i, result);
    }

    for (int i = size - 1; i > 0; --i) {
        std::swap(arr[0], arr[i]);
        result.stats.swaps += 1;
        addArrayStep(result, arr, "Moved the largest value to the end of the heap.", 0, i, -1, 0, i - 1);
        heapify(arr, i, 0, result);
    }
}

int linearSearch(const std::vector<int>& arr, int target, RunResult& result) {
    for (size_t i = 0; i < arr.size(); ++i) {
        result.stats.comparisons += 1;
        addArrayStep(result, arr, "Checking the next value.", static_cast<int>(i));
        if (arr[i] == target) {
            addArrayStep(result, arr, "Target found.", static_cast<int>(i), -1, static_cast<int>(i));
            return static_cast<int>(i);
        }
    }
    return -1;
}

int binarySearch(const std::vector<int>& arr, int target, RunResult& result) {
    int low = 0;
    int high = static_cast<int>(arr.size()) - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        result.stats.comparisons += 1;
        addArrayStep(result, arr, "Checking the middle value of the current search range.", mid, -1, -1, low, high);

        if (arr[mid] == target) {
            addArrayStep(result, arr, "Target found.", mid, -1, mid, low, high);
            return mid;
        }

        result.stats.comparisons += 1;
        if (arr[mid] < target) {
            low = mid + 1;
            addArrayStep(result, arr, "Target is larger, so the search continues on the right side.", mid, -1, -1, low, high);
        } else {
            high = mid - 1;
            addArrayStep(result, arr, "Target is smaller, so the search continues on the left side.", mid, -1, -1, low, high);
        }
    }

    return -1;
}

RunResult runSorting(const std::string& algorithm, std::vector<int> values) {
    RunResult result;
    result.algorithm = algorithm;
    result.category = "sorting";
    addArrayStep(result, values, "Initial array.");

    if (algorithm == "bubble") {
        bubbleSort(values, result);
    } else if (algorithm == "selection") {
        selectionSort(values, result);
    } else if (algorithm == "insertion") {
        insertionSort(values, result);
    } else if (algorithm == "merge") {
        if (!values.empty()) {
            mergeSort(values, 0, static_cast<int>(values.size()) - 1, result);
        }
    } else if (algorithm == "quick") {
        if (!values.empty()) {
            quickSort(values, 0, static_cast<int>(values.size()) - 1, result);
        }
    } else if (algorithm == "heap") {
        heapSort(values, result);
    }

    result.finalArray = values;
    addArrayStep(result, values, "Sorting complete.");
    return result;
}

RunResult runSearching(const std::string& algorithm, std::vector<int> values, int target) {
    RunResult result;
    result.algorithm = algorithm;
    result.category = "searching";
    addArrayStep(result, values, "Initial array.");

    if (algorithm == "binary") {
        std::sort(values.begin(), values.end());
        addArrayStep(result, values, "Binary search requires a sorted array, so the values were sorted first.");
    }

    result.resultIndex = (algorithm == "binary")
        ? binarySearch(values, target, result)
        : linearSearch(values, target, result);

    result.finalArray = values;
    if (result.resultIndex >= 0) {
        addArrayStep(result, values, "Search complete. The target was found.", result.resultIndex, -1, result.resultIndex);
    } else {
        addArrayStep(result, values, "Search complete. The target was not found.");
    }
    return result;
}

RunResult runTree(std::vector<int> values) {
    RunResult result;
    result.algorithm = "avl";
    result.category = "tree";

    Node* root = nullptr;
    for (int value : values) {
        root = insertAvl(root, value, result);
        addTreeStep(result, root, "Inserted " + std::to_string(value) + " into the AVL tree.");
    }

    result.finalArray = values;
    destroyTree(root);
    return result;
}

std::string resultToJson(const RunResult& result) {
    std::ostringstream out;
    out << "{";
    out << "\"algorithm\":\"" << escapeJson(result.algorithm) << "\",";
    out << "\"category\":\"" << escapeJson(result.category) << "\",";
    out << "\"resultIndex\":" << result.resultIndex << ",";
    out << "\"stats\":{";
    out << "\"steps\":" << result.stats.steps << ",";
    out << "\"comparisons\":" << result.stats.comparisons << ",";
    out << "\"swaps\":" << result.stats.swaps;
    out << "},";
    out << "\"finalArray\":" << vectorToJson(result.finalArray) << ",";
    out << "\"steps\":[";

    for (size_t i = 0; i < result.steps.size(); ++i) {
        const Step& step = result.steps[i];
        if (i > 0) {
            out << ",";
        }
        out << "{";
        out << "\"type\":\"" << escapeJson(step.type) << "\",";
        out << "\"array\":" << vectorToJson(step.array) << ",";
        out << "\"current\":" << step.current << ",";
        out << "\"secondary\":" << step.secondary << ",";
        out << "\"found\":" << step.found << ",";
        out << "\"low\":" << step.low << ",";
        out << "\"high\":" << step.high << ",";
        out << "\"pivot\":" << step.pivot << ",";
        out << "\"message\":\"" << escapeJson(step.message) << "\",";
        out << "\"treeLevels\":" << treeLevelsToJson(step.treeLevels) << ",";
        out << "\"treeValues\":" << vectorToJson(step.treeValues) << ",";
        out << "\"treePresent\":" << vectorToJson(step.treePresent);
        out << "}";
    }

    out << "]";
    out << "}";
    return out.str();
}

char* duplicateCString(const std::string& text) {
    char* buffer = static_cast<char*>(std::malloc(text.size() + 1));
    if (!buffer) {
        return nullptr;
    }
    std::copy(text.begin(), text.end(), buffer);
    buffer[text.size()] = '\0';
    return buffer;
}

}  // namespace

extern "C" {

// This function is exported to JavaScript.
// JavaScript sends the algorithm name and the array values as a CSV string.
// The C++ code runs the algorithm, records every visual step, and returns JSON.
char* run_algorithm(const char* category, const char* algorithm, const char* valuesCsv, int target) {
    try {
        const std::string categoryText = category ? category : "";
        const std::string algorithmText = algorithm ? algorithm : "";
        const std::vector<int> values = parseCsvNumbers(valuesCsv ? valuesCsv : "");

        RunResult result;
        if (categoryText == "sorting") {
            result = runSorting(algorithmText, values);
        } else if (categoryText == "searching") {
            result = runSearching(algorithmText, values, target);
        } else if (categoryText == "tree") {
            result = runTree(values);
        } else {
            result.algorithm = algorithmText;
            result.category = categoryText;
        }

        return duplicateCString(resultToJson(result));
    } catch (const std::exception& ex) {
        std::string errorJson = std::string("{\"error\":\"") + escapeJson(ex.what()) + "\"}";
        return duplicateCString(errorJson);
    } catch (...) {
        return duplicateCString("{\"error\":\"Unknown C++ error.\"}");
    }
}

// JavaScript calls this after reading the returned JSON string.
// This avoids leaking memory inside the WebAssembly module.
void free_result(char* ptr) {
    std::free(ptr);
}

}
