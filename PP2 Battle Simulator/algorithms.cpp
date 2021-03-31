#include "algorithms.h"
#include <cmath>
#include "precomp.h"

using namespace std;

namespace Tmpl8
{
	//Countsort for Healthbars
	vector<int> CountSort(const vector<Tank*>& in)
	{
		vector<int> counters(TANK_MAX_HEALTH + 1, 0);
		vector<int> result;

		for (auto x : in)
			counters.at(x->health <= 0 ? 0 : x->health)++;

		for (int i = 0; i < TANK_MAX_HEALTH + 1; ++i)
			if (counters[i] != 0)
				for (int y = 0; y < counters[i]; ++y)
					result.push_back(i);

		return result;
	}

	template <class T>
	void LinkedList<T>::InsertValue(T value)
	{
		auto* new_node = new Node<T>(value);

		if (head == nullptr || value <= head->value)
		{
			new_node->next = head;
			head = new_node;
			return;
		}

		Node<T>* current = head;
		while (current->next != nullptr && value >= current->next->value) { current = current->next; }

		//Add node
		new_node->next = current->next;
		current->next = new_node;
	}

	// -----------------------------------------------------------
	// Sort tanks by health value using bucket sort
	// -----------------------------------------------------------
	template <>
	vector<LinkedList<int>> LinkedList<int>::Sort(vector<Tank*>& input, int n_buckets)
	{
		vector<LinkedList<int>> buckets(n_buckets);
		for (auto tank : input) { buckets.at(tank->health / n_buckets).InsertValue(tank->health); }
		return buckets;
	}

	KDTree::KDTree(std::vector<Tank*>& input)
	{
		std::vector<Tank*> activeTanks = {};

		// only use active tanks for building the KD tree
		for (auto tank : input)
			if (tank->active)
				activeTanks.emplace_back(tank);

		root = BuildKDTree(activeTanks, 0);
	}
	// Inserts list of tanks in the tree and return the root
	// The parameter depth is used to decide axis of comparison
	KDNode* KDTree::BuildKDTree(std::vector<Tank*> input, unsigned depth)
	{
		// Tree is empty?
		if (input.size() == 1)
			return new KDNode(input[0]);

		if (input.empty())
			return nullptr;

		unsigned axis = depth % 2;

		// Sort input based on current depth
		sort(input.begin(), input.end(), [axis](Tank* a, Tank* b) {
			return a->position[axis] < b->position[axis];
			});

		Tank* tank = input[input.size() / 2];
		input.erase(input.begin() + (input.size() / 2));
		vector<Tank*> left(input.begin(), input.begin() + (input.size() / 2));
		vector<Tank*> right(input.begin() + ((input.size() / 2) + 1), input.end());

		KDNode* root = new KDNode(tank);
		root->left = BuildKDTree(left, depth + 1);
		root->right = BuildKDTree(right, depth + 1);

		return root;
	}

	// Searches the closest enemy tank in the K D tree.
	Tank* KDTree::find_closest_enemy(Tank* tank)
	{
		// some tanks go outside the screen, that is why we add some margin.
		float errorMargin = 250.f;
		float max = numeric_limits<float>::infinity();
		Rectangle2D hyperplane = { {-250.f, -250.f}, {1750.f, 1750.f} };

		// root is at depth of 0
		return searchNN(root, tank, hyperplane, max, nullptr, 0);
	}

	Tank* KDTree::searchNN(KDNode* currentNode, Tank* target, Rectangle2D& hyperplane, float distanceCurrentClosestTank, Tank* currentClosestTank, int depth)
	{
		if (currentNode == nullptr)
			return currentClosestTank;

		// X[0], Y[1] axis
		int axis = depth % 2;
		Rectangle2D leftOrTopHyperplane = {}, rightOrBottomHyperplane = {}, closestHyperplane = {}, furthestHyperplane = {};
		KDNode* closestNode = nullptr, * furthestNode = nullptr;

		// X axis, divide vertical
		if (axis == 0)
		{
			leftOrTopHyperplane = { hyperplane.min, {currentNode->tank->position.x, hyperplane.max.y} };
			rightOrBottomHyperplane = { {currentNode->tank->position.x, hyperplane.min.y}, hyperplane.max };
		}
		// Y axis, divide horizontal
		if (axis == 1)
		{
			leftOrTopHyperplane = { hyperplane.min, {hyperplane.max.x, currentNode->tank->position.y} };
			rightOrBottomHyperplane = { {hyperplane.min.x, currentNode->tank->position.y}, hyperplane.max };
		}
		// check which hyperplane the target(tank that's firing) belongs to
		if (target->position[axis] <= currentNode->tank->position[axis])
		{
			closestNode = currentNode->left;
			furthestNode = currentNode->right;
			closestHyperplane = leftOrTopHyperplane;
			furthestHyperplane = rightOrBottomHyperplane;
		}
		if (target->position[axis] > currentNode->tank->position[axis])
		{
			closestNode = currentNode->right;
			furthestNode = currentNode->left;
			closestHyperplane = rightOrBottomHyperplane;
			furthestHyperplane = leftOrTopHyperplane;
		}

		// check if the current node is closer to the target
		float dist = pow(currentNode->tank->position.x - target->position.x, 2) + pow(currentNode->tank->position.y - target->position.y, 2);
		if (dist < distanceCurrentClosestTank)
		{
			currentClosestTank = currentNode->tank;
			distanceCurrentClosestTank = dist;
		}

		// go deeper into the tree
		Tank* closestTank = searchNN(closestNode, target, closestHyperplane, distanceCurrentClosestTank, currentClosestTank, depth + 1);

		float distanceClosestTank = 0;
		dist = pow(closestTank->position.x - target->position.x, 2) + pow(closestTank->position.y - target->position.y, 2);
		if (distanceCurrentClosestTank < dist)
		{
			closestTank = currentClosestTank;
			distanceClosestTank = distanceCurrentClosestTank;
		}

		float pointX = calculateCurrentClosest(target->position.x, furthestHyperplane.min.x, furthestHyperplane.max.x);
		float pointY = calculateCurrentClosest(target->position.y, furthestHyperplane.min.y, furthestHyperplane.max.y);

		dist = pow((pointX - target->position.x), 2) + pow((pointY - target->position.y), 2);

		if (dist < distanceClosestTank)
			closestTank = searchNN(furthestNode, target, furthestHyperplane, distanceCurrentClosestTank, currentClosestTank, depth + 1);
		return closestTank;
	}
	float KDTree::calculateCurrentClosest(float targetXY, float hyperplaneMinXY, float hyperplaneMaxXY)
	{
		float value = 0;
		if (hyperplaneMinXY < targetXY && targetXY > hyperplaneMaxXY)
			value = targetXY;
		else if (targetXY <= hyperplaneMinXY)
			value = hyperplaneMinXY;
		else if (targetXY >= hyperplaneMaxXY)
			value = hyperplaneMaxXY;

		return value;
	}

} // namespace Tmpl8