#ifndef TRUCK_CPP
#define TRUCK_CPP

#include "Truck.h"

Truck::Truck()
{
}

Truck::Truck(TRUCKTYPE truck_type, int capacity, Time checkUpTime, int journeysBeforeCheckUp, double speed, int id)
{
	this->truck_type = truck_type;
	this->capacity = capacity;
	this->checkUpTime = checkUpTime;
	this->speed = speed;
	this->jounrneysBeforeCheckUp = journeysBeforeCheckUp;
	this->ID = id;
	this->CalculateDeliveryInterval();
}

Time Truck::GetCheckUpTime() const
{
	return this->checkUpTime;
}

TRUCKTYPE Truck::GetTruckType() const
{
	return truck_type;
}

int Truck::GetCapacity() const
{
	return this->capacity;
}

double Truck::GetSpeed() const
{
	return this->speed;
}

Time Truck::GetDeliveryInterval() const
{
	return this->deliveryInterval;
}

int Truck::GetJourneysBeforeCheckUp() const
{
	return this->jounrneysBeforeCheckUp;
}

void Truck::CalculateDeliveryInterval()
{
	int totalLoadTime = 0;
	double maxDeliveryDistance = 0;

	Node<Cargo*>* curr = this->cargos.GetHead();
	
	while(curr != nullptr){
		if(maxDeliveryDistance < curr->getItem()->GetDeliveryDistance()){
			maxDeliveryDistance = curr->getItem()->GetDeliveryDistance();
		}

		totalLoadTime += curr->getItem()->GetLoadTime();
		curr = curr->getNext();
	}

	this->deliveryInterval = 2* (maxDeliveryDistance/this->speed) + totalLoadTime;

}

int Truck::GetID() const
{
	return this->ID;
}

void Truck::SetID(int id)
{
	this->ID = id;
}

std::ostream& operator<<(std::ostream& os , const Truck* truck)
{
	os << truck->GetID();
	return os;
}


std::ostream& operator<<(std::ostream& os, const Truck truck) 
{
	os << truck.GetID();

	return os;
}

#endif