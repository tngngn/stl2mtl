#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <regex>
#include <set>
#include <algorithm>
#include <cmath>

// Function to extract atomic propositions from the STL formula
std::vector<std::string> extractAtomicPropositions(const std::string& stlFormula) {
    std::vector<std::string> atomicProps;
    std::regex propPattern(R"((\w+\s*[<>]=?\s*[\d\.]+))"); // Match expressions like y < 2, z > 1, x >= 0.3
    std::sregex_iterator it(stlFormula.begin(), stlFormula.end(), propPattern);
    std::sregex_iterator end;
    
    while (it != end) {
        atomicProps.push_back(it->str());
        ++it;
    }
    return atomicProps;
}

// Function to replace real-valued atomic propositions with Boolean ones
std::string replaceAtomicProps(const std::string& stlFormula, const std::map<std::string, std::string>& propMap) {
    std::string mitlFormula = stlFormula;
    for (const auto& pair : propMap) {
        mitlFormula = std::regex_replace(mitlFormula, std::regex("\\b" + pair.first + "\\b"), pair.second);
    }
    return mitlFormula;
}

// Function to write the MITL formula to a .mitl file
void writeMITLToFile(const std::string& mitlFormula, std::string filename) {
    if (filename.find(".mitl") == std::string::npos) {
        filename += ".mitl";  // Ensure file extension is added
    }
    
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << mitlFormula;
        outFile.close();
        std::cout << "MITL formula written to " << filename << std::endl;
    } else {
        std::cerr << "Error: Unable to write to file " << filename << std::endl;
    }
}

// Function to synthesize a signal based on the given behavior
std::vector<std::pair<double, std::vector<bool>>> synthesizeSignal(double T) {
    std::vector<std::pair<double, std::vector<bool>>> signal;

    // Define the signal behavior
    auto y = [](double t) { return t < 10; }; // y(t) < 2 for t ∈ [0, 10)
    auto z = [](double t) { return t >= 5 && t < 15; }; // z(t) > 1 for t ∈ [5, 15)
    auto x = [](double t) { return t >= 8 && t < 20; }; // x(t) > 0.3 for t ∈ [8, 20)

    // Generate the signal
    for (double t = 0; t <= T; t += 0.1) {
        signal.push_back({t, {y(t), z(t), x(t)}});
    }

    return signal;
}

// Function to construct stable partitions for each atomic proposition
std::set<int> constructStablePartitions(const std::vector<std::pair<double, std::vector<bool>>>& signal) {
    std::set<int> partitionPoints;

    // Add boundaries where the truth value of any atomic proposition changes
    for (size_t i = 1; i < signal.size(); ++i) {
        if (signal[i].second != signal[i - 1].second) {
            partitionPoints.insert(static_cast<int>(std::round(signal[i].first)));
        }
    }

    return partitionPoints;
}

// Function to partition temporal operators based on stable partition points
std::string partitionTemporalOperators(const std::string& mitlFormula, const std::set<int>& partitionPoints) {
    std::string partitionedFormula = mitlFormula;

    // Replace G [a, b] ((p2) U (p3)) with G [a, t1] ((p2) U (p3)) ∧ G [t1+1, t2] ((p2) U (p3)) ∧ ... ∧ G [tn, b] ((p2) U (p3))
    std::regex gPattern(R"(\bG\s*\[\s*([\d\.]+)\s*,\s*([\d\.]+)\s*\]\s*\(\(p2\)\s*U\s*\(p3\)\))");
    std::smatch match;
    if (std::regex_search(partitionedFormula, match, gPattern)) {
        int a = static_cast<int>(std::stod(match[1]));
        int b = static_cast<int>(std::stod(match[2]));
        std::string replacement;

        auto it = partitionPoints.lower_bound(a);
        int prev = a;
        while (it != partitionPoints.end() && *it <= b) {
            int t = *it;
            if (prev <= t) {
                replacement += "G [" + std::to_string(prev) + ", " + std::to_string(t) + "] ((p2) U (p3)) ";
                if (t < b) {
                    replacement += "∧ ";
                }
            }
            prev = t + 1;
            ++it;
        }
        if (prev <= b) {
            replacement += "G [" + std::to_string(prev) + ", " + std::to_string(b) + "] ((p2) U (p3)) ";
        }

        partitionedFormula = std::regex_replace(partitionedFormula, gPattern, replacement);
    }

    return partitionedFormula;
}

int main() {
    // Step 1: Input STL formula
    std::string stlFormula;
    std::cout << "Enter the STL formula: ";
    std::getline(std::cin, stlFormula);

    // Step 2: Extract atomic propositions
    std::vector<std::string> atomicProps = extractAtomicPropositions(stlFormula);
    std::cout << "\nStep 1: Extracted atomic propositions:\n";
    for (const auto& prop : atomicProps) {
        std::cout << "- " << prop << std::endl;
    }

    // Step 3: Map atomic propositions to Boolean variables
    std::map<std::string, std::string> propMap;
    for (size_t i = 0; i < atomicProps.size(); ++i) {
        propMap[atomicProps[i]] = "p" + std::to_string(i + 1);
    }

    std::cout << "\nStep 2: Mapped atomic propositions to Boolean variables:\n";
    for (const auto& pair : propMap) {
        std::cout << "- " << pair.first << " -> " << pair.second << std::endl;
    }

    // Step 4: Synthesize a signal
    double T = 30; // Time horizon
    auto signal = synthesizeSignal(T);
    std::cout << "\nStep 3: Synthesized signal behavior:\n";
    for (const auto& point : signal) {
        std::cout << "t = " << point.first << ", (y < 2, z > 1, x > 0.3) = ("
                  << point.second[0] << ", " << point.second[1] << ", " << point.second[2] << ")\n";
    }

    // Step 5: Construct stable partitions
    auto partitionPoints = constructStablePartitions(signal);
    std::cout << "\nStep 4: Constructed stable partitions:\n";
    for (int t : partitionPoints) {
        std::cout << "Partition point: " << t << std::endl;
    }

    // Step 6: Replace atomic propositions in the STL formula
    std::string mitlFormula = replaceAtomicProps(stlFormula, propMap);
    std::cout << "\nStep 5: Replaced atomic propositions in the STL formula:\n";
    std::cout << "STL Formula: " << stlFormula << std::endl;
    std::cout << "MITL Formula (before partitioning): " << mitlFormula << std::endl;

    // Step 7: Partition temporal operators
    mitlFormula = partitionTemporalOperators(mitlFormula, partitionPoints);
    std::cout << "\nStep 6: Partitioned temporal operators in the MITL formula:\n";
    std::cout << "MITL Formula (after partitioning): " << mitlFormula << std::endl;

    // Step 8: Write the MITL formula to a .mitl file
    std::string filename;
    std::cout << "\nStep 7: Enter the filename to save the MITL formula (e.g., output): ";
    std::cin >> filename;
    writeMITLToFile(mitlFormula, filename);

    return 0;
}