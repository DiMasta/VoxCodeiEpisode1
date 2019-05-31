#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <set>

using namespace std;

constexpr int MAX_SETS = 50;

int main() {
	set<int> s1 { 1, 2, 5 };
	set<int> s2 { 2, 3, 4 };
	set<int> s3 { 1, 3 };

	constexpr int SETS_COUNT = 3;

	set<int> S[MAX_SETS] { s1, s2, s3 };

	set<set<int>> T;
	for (int e : S[0]) {
		set<int> eSet { e };

		T.insert(eSet);
	}

	for (int setIdx = 1; setIdx < SETS_COUNT; ++setIdx) {
		const set<int>& s = S[setIdx];

		set<set<int>> newT;
		for (const set<int>& tSet : T) {
			for (int e : s) {
				set<int> newTSet = tSet;
				newTSet.insert(e);

				newT.insert(newTSet);
			}
		}

		// Transversal newT
		set<set<int>> containingSubset;
		for (const set<int>& subset : newT) {
			for (const set<int>& containsSubset : newT) {
				if (subset != containsSubset) {
					const bool isSubset = includes(containsSubset.begin(), containsSubset.end(), subset.begin(), subset.end());

					if (isSubset) {
						containingSubset.insert(containsSubset);
					}
				}
			}
		}

		for (const set<int>& hasSubset : containingSubset) {
			newT.erase(hasSubset);
		}

		T = newT;
	}

	return 0;
}