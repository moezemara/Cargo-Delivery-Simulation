#ifndef  Comapny_CPP
#define Comapny_CPP

#include "Company.h"


Company::Company() {
	this->TimestepNum = Time();

	this->pUI = new UI;


	//getting the input & output file names from the UI class
	this->inputFileName = this->pUI->GetInputFilePath();
	this->outputFileName = this->pUI->GetOutputFilePath();


	// initialize load function
	this->LoadInputs();
}

Company::~Company() {

	this->SaveOutputs();

	delete this->pUI;

	// Delete The Event List
	Event* EventTempPtr = nullptr;

	this->cleanQueueInnerPointers(this->EventList);
	delete this->EventList;

	// Delete The Cargos Lists
	Node<Cargo*>* CargoNodeTempPtr = nullptr;

	CargoNodeTempPtr = NormalCargoList->GetHead();
	while (CargoNodeTempPtr != nullptr) {
		delete CargoNodeTempPtr->getItem();
		CargoNodeTempPtr = CargoNodeTempPtr->getNext();
	}

	this->cleanQueueInnerPointers(this->SpecialCargoList);
	this->cleanPriorityQueueInnerPointers(this->VIPCargoList);

	delete this->NormalCargoList;
	delete this->SpecialCargoList;
	delete this->VIPCargoList;

	this->cleanQueueInnerPointers(this->DeliveredNormalCargoList);
	this->cleanQueueInnerPointers(this->DeliveredSpecialCargoList);
	this->cleanQueueInnerPointers(this->DeliveredVIPCargoList);


	delete this->DeliveredNormalCargoList;
	delete this->DeliveredSpecialCargoList;
	delete this->DeliveredVIPCargoList;

	// Delete The Trucks Lists
	this->cleanQueueInnerPointers(this->NormalTrucksList);
	this->cleanQueueInnerPointers(this->SpecialTrucksList);
	this->cleanQueueInnerPointers(this->VIPTrucksList);
	this->cleanPriorityQueueInnerPointers(this->InCheckUpTrucksList);
	
	delete this->NormalTrucksList;
	delete this->SpecialTrucksList;
	delete this->VIPTrucksList;
	delete this->InCheckUpTrucksList;


	// Delete The Maintenance Lists
	this->cleanQueueInnerPointers(this->NormalMaintenanceTrucksList);
	this->cleanQueueInnerPointers(this->SpecialMaintenanceTrucksList);
	this->cleanQueueInnerPointers(this->VIPMaintenanceTrucksList);

	delete this->NormalMaintenanceTrucksList;
	delete this->SpecialMaintenanceTrucksList;
	delete this->VIPMaintenanceTrucksList;
}

bool Company::CheckExitStatus() {
	// check for application status 
	// The simulation function stops when there are no more events and all cargos are in delivered list

	return (this->NormalCargoList->isEmpty() && this->SpecialCargoList->isEmpty() && this->VIPCargoList->isEmpty() && this->EventList->isEmpty());
}

void Company::Simulate() {
	while (true) {
		this->TimestepNum = this->TimestepNum + 1;

		// Execute the upcoming event
		this->ExecuteUpcomingEvent();

		// move trucks from checkup to available
		this->MoveCheckUpToAvailable();

		// move trucks from available to checkup
		this->CheckForCheckUp();

		// move trucks from maintenance to available
		this->MoveMaintenanceToAvailable();

		// Todo: account for the max time waiting rule
		this->LoadVIPCargosToTruck();
		this->LoadSpecialCargosToTruck();
		this->LoadNormalCargosToTruck();

		// Move Trucks to the moving trucks list if applicable
		this->MoveTrucks();

		// print current info
		this->UpdateInterface();

		//check for Auto Promote
		this->checkForAutoPromote();

		//check break conditions
		if (this->CheckExitStatus())
		{

			//making data for the output file



			this->SaveOutputs();


			this->pUI->PrintMsg("Simulation is done.");
			break;
		}
	}
}

void Company::LoadInputs() {
	using std::ifstream;
	ifstream inputFile(this->inputFileName);

	//checking that the stream open successfully 
	if (!inputFile.is_open()) {
		this->pUI->PrintMsg("*** Error: Could not open " + this->inputFileName + " ***");
		exit(1);
	}

	/////////////////// Loading Trucks //////////////////////////

	int nTrucksNum, sTrucksNum, vTrucksNum;
	double nTruckSpeed, sTruckSpeed, vTruckSpeed;
	int nCapacity, sCapacity, vCapacity;
	int JourNum;
	int nCheckUpHours, sCheckUpHours, vCheckUpHours;

	//reading parameters from the file 
	inputFile >> nTrucksNum >> sTrucksNum >> vTrucksNum;
	inputFile >> nTruckSpeed >> sTruckSpeed >> vTruckSpeed;
	inputFile >> nCapacity >> sCapacity >> vCapacity;

	inputFile >> JourNum;
	inputFile >> nCheckUpHours >> sCheckUpHours >> vCheckUpHours;


	//adding VIP trucks
	for (int i = 0; i < vTrucksNum; i++)
		this->AddTruck(TRUCKTYPE::VT, vCapacity, Time(vCheckUpHours), JourNum, vTruckSpeed, i);

	//adding special trucks
	for (int i = 0; i < sTrucksNum; i++)
		this->AddTruck(TRUCKTYPE::ST, sCapacity, Time(sCheckUpHours), JourNum, sTruckSpeed, i);

	//adding normal trucks
	for (int i = 0; i < sTrucksNum; i++)
		this->AddTruck(TRUCKTYPE::NT, nCapacity, Time(nCheckUpHours), JourNum, nTruckSpeed, i);


	////////////////// Reading Auto Promotion Limit & Maximum waiting hours ////////////////
	int Apl, MaxW;
	inputFile >> Apl >> MaxW;

	this->AutoPromotionLimit = Time(Apl, 0);
	this->MaxWaitingTime = Time(MaxW);


	///////////////// Loading events ///////////////////

	//reading events from the file





	/// loops on each event and takes the letter to check which event function to call.
	/// based on this the event function will take the right number of params for it
	/// Ready : 6 params
	/// Cancellation: 2 params
	/// Promotional: 3 params

	/// <summary>
	/// This reads Line 8 in input file
	/// Based on the number of file there will be number of loops below
	/// </summary>
	inputFile >> NumOfEvents;

	for (int i = 0; i < NumOfEvents; i++) {
		char EventChar;
		inputFile >> EventChar;

		/// <summary>
		/// based on the the first letter in the line, it will go on the corresponding function.
		/// it will continue reading the file by passing the file by reference.
		/// </summary>
		switch (EventChar)
		{
		case 'R':
			ReadReadyEvent(inputFile);
			break;
		case 'P':
			ReadPromotionEvent(inputFile);
			break;
		case 'X':
			ReadCancellationEvent(inputFile);
			break;
		}

	}

	this->pUI->PrintMsg("Input file successfully loaded!");
	inputFile.close();
}


/// These function will continue reading from the file

void Company::ReadReadyEvent(std::ifstream& inputFile)
{
	char sCType;
	inputFile >> sCType;
	CARGOTYPE CType;
	if (sCType == 'N')
		CType = CARGOTYPE::N;
	else if (sCType == 'V')
		CType = CARGOTYPE::V;
	else if (sCType == 'S')
		CType = CARGOTYPE::S;

	string sEventTime;
	inputFile >> sEventTime;
	Time EventTime = splitTime(sEventTime);

	int ID;
	inputFile >> ID;
	int Distance;
	inputFile >> Distance;
	int LT;
	inputFile >> LT;
	int Cost;
	inputFile >> Cost;


	Event* pEvent = new ReadyEvent(this, EventTime, ID, CType, Distance, LT, Cost);
	this->AddEvent(pEvent);
}

void Company::ReadCancellationEvent(std::ifstream& inputFile)
{
	string sEventTime;
	inputFile >> sEventTime;
	string TimeList[2];

	Time EventTime = splitTime(sEventTime);

	int ID;
	inputFile >> ID;
	Event* pEvent = new CancellationEvent(this, EventTime, ID);
	this->AddEvent(pEvent);
}

void Company::ReadPromotionEvent(std::ifstream& inputFile)
{
	string sEventTime;
	inputFile >> sEventTime;
	string TimeList[2];

	Time EventTime = splitTime(sEventTime);

	int ID;
	inputFile >> ID;
	float ExtraMoney;
	inputFile >> ExtraMoney;
	Event* pEvent = new PromotionEvent(this, EventTime, ID, ExtraMoney);
	this->AddEvent(pEvent);
}


CARGOTYPE whichIsFirst(Cargo* normal, Cargo* vip, Cargo* special) {

	Cargo* first_delivered_Cargo = normal;
	CARGOTYPE type = CARGOTYPE::N;
	
	if (vip->GetDeliveredTime() < first_delivered_Cargo->GetDeliveredTime()) {
		first_delivered_Cargo = vip;
		type = CARGOTYPE::V;
	}
	if (special->GetDeliveredTime() < first_delivered_Cargo->GetDeliveredTime()) {
		first_delivered_Cargo = special;
		type = CARGOTYPE::S;
	}

	return type;

	return CARGOTYPE::N;
}

/// There was AddEvents function here and I replaced it with ReadEvents
/// purpose: to read from files and then call the Event class
/// if it has different purpose, add it again 


void Company::SaveOutputs() {

	// called on exit
	std::ofstream outputFile(this->outputFileName);

	Time avgWaitinngTime;

	int WaitingCargosCount, LoadingTrucksCount, EmptyTrucksCount, MovingCargosCout,
		InCheckupTrucksCount, DeliveredCargosCount;

	WaitingCargosCount = this->NormalCargoList->getCount() +
		this->SpecialCargoList->getCount() +
		this->VIPCargoList->getCount();

	LoadingTrucksCount = this->NormalTrucksList->getCount() +
		this->SpecialTrucksList->getCount() +
		this->VIPTrucksList->getCount();

	DeliveredCargosCount = this->DeliveredNormalCargoList->getCount() +
		this->DeliveredSpecialCargoList->getCount() +
		this->DeliveredVIPCargoList->getCount();



	string dataToOutput = "";

	while (!DeliveredNormalCargoList->isEmpty() ||
		!DeliveredVIPCargoList->isEmpty() ||
		!DeliveredSpecialCargoList->isEmpty() )
	{

		Cargo* normal;
		this->DeliveredNormalCargoList->peek(normal);
		Cargo* vip;
		this->DeliveredVIPCargoList->peek(vip);
		Cargo* special;
		this->DeliveredSpecialCargoList->peek(special);

		CARGOTYPE type = whichIsFirst(normal, vip, special);

		Cargo* cargo;
		switch (type)
		{
		case CARGOTYPE::S:
			this->DeliveredSpecialCargoList->dequeue(cargo);
			break;
		case CARGOTYPE::V:
			this->DeliveredVIPCargoList->dequeue(cargo);
			break;

		default://case CARGOTYPE::N:
			this->DeliveredNormalCargoList->dequeue(cargo);
			break;
		}

		avgWaitinngTime = avgWaitinngTime + cargo->GetWaitingTime();
		
		dataToOutput += cargo->GetDeliveredTime().StringifyTime() + "\t" +
			std::to_string(cargo->GetID()) + "\t" +
			cargo->GetWaitingTime().StringifyTime() + "\t" +
			std::to_string(cargo->GetTruckID()) + "\n" +
			"";
	}


	//output the file here
	outputFile << "-----------------------------------------"<<endl;



	outputFile.close();
}

void Company::AddTruck(TRUCKTYPE truck_type, int capacity, Time checkUpTime, int journeysBeforeCheckUp, double speed, int id)
{
	Truck* truck = new Truck(truck_type, capacity, checkUpTime, journeysBeforeCheckUp, speed, id);
	switch (truck_type)
	{
	case TRUCKTYPE::NT:
		this->NormalTrucksList->enqueue(truck);
		break;
	case TRUCKTYPE::ST:
		this->SpecialTrucksList->enqueue(truck);
		break;
	case TRUCKTYPE::VT:
		this->VIPTrucksList->enqueue(truck);
		break;
	}
}

void Company::UpdateInterface() {
	// Run the appropriate interface function based on the current mode.
	switch (this->pUI->GetAppMode())
	{
	case MODE::INTER:
		pUI->InteractiveInterfaceUpdate(this->GetCurrentTime(), this->GetInteractiveModeData());
		break;
	case MODE::STEP:
		pUI->StepInterfaceUpdate();
		break;
	case MODE::SILENT:
		pUI->SilentInterfaceUpdate();
		break;
	}
}

std::string Company::GetInteractiveModeData() const {
	string interactive_mode_data;
	string separator = "\n--------------------------------------------------\n";

	int WaitingCargosCount, LoadingTrucksCount, EmptyTrucksCount, MovingCargosCount,
		InCheckupTrucksCount, DeliveredCargosCount;

	WaitingCargosCount = this->NormalCargoList->getCount() +
		this->SpecialCargoList->getCount() +
		this->VIPCargoList->getCount();

	LoadingTrucksCount = this->NormalTrucksList->getCount() +
		this->SpecialTrucksList->getCount() +
		this->VIPTrucksList->getCount();

	MovingCargosCount = this->MovingTrucks->getCount();

	DeliveredCargosCount = this->DeliveredNormalCargoList->getCount() +
		this->DeliveredSpecialCargoList->getCount() +
		this->DeliveredVIPCargoList->getCount();


	// Waiting Cargos Line:
	interactive_mode_data += WaitingCargosCount + " Waiting Cargos: ";
	interactive_mode_data += "[" + this->NormalCargoList->getData() + "] ";
	interactive_mode_data += "(" + this->SpecialCargoList->getData() + ") ";
	interactive_mode_data += "{" + this->VIPCargoList->getData() + "}";
	interactive_mode_data += separator;

	interactive_mode_data += DeliveredCargosCount + " Delivered Cargos: ";
	interactive_mode_data += "[" + this->DeliveredNormalCargoList->getData() + "] ";
	interactive_mode_data += "(" + this->DeliveredSpecialCargoList->getData() + ") ";
	interactive_mode_data += "{" + this->DeliveredVIPCargoList->getData() + "}";
	interactive_mode_data += separator;

	return interactive_mode_data;
}

void Company::AddEvent(Event* pEvent) {
	this->EventList->enqueue(pEvent);
}

void Company::AddWaitCargo(Cargo* pCargo) {
	CARGOTYPE cargo_type = pCargo->GetType();
	switch (cargo_type)
	{
	case CARGOTYPE::N:
		this->NormalCargoList->Insert(pCargo);
		break;
	case CARGOTYPE::S:
		this->SpecialCargoList->enqueue(pCargo);
		break;
	case CARGOTYPE::V:
		this->AddVIPCargo(pCargo);
		break;

	}
}

Cargo* Company::FindNormalCargo(int ID) {
	Node<Cargo*>* Head = this->NormalCargoList->GetHead();

	while (Head != nullptr) {
		if (Head->getItem()->GetID() == ID) {
			return Head->getItem();
		}
		Head = Head->getNext();
	}

	return nullptr;
}

/*
	- Can be optimized.
*/

void Company::DeleteNormalCargo(int ID) {

	Node<Cargo*>* loopingPtr = this->NormalCargoList->GetHead();
	Node<Cargo*>* prevPtr = loopingPtr;
	if (this->NormalCargoList->GetHead() == loopingPtr) {
		this->NormalCargoList->SetHead(loopingPtr->getNext());
		delete loopingPtr;
		this->NormalCargoList->DecreaseCount();
		loopingPtr = nullptr;
		return;
	}

	while (loopingPtr != nullptr) {
		if (loopingPtr->getItem()->GetID() == ID) {
			prevPtr->setNext(loopingPtr->getNext());
			delete loopingPtr;
			loopingPtr = nullptr;
			return;
		}
	}

}

/*
	- Priority doesn't only depend on the cost. *Need to change*
*/
void Company::AddVIPCargo(Cargo* pCargo) {

	double priority_calc = pCargo->GetCost() / 2000
		- pCargo->GetDeliveryDistance() / 2000
		- pCargo->GetPrepTime().GetTotalHours() / 5
		- pCargo->GetLoadTime() / 10;


	this->VIPCargoList->enqueue(pCargo, priority_calc);
}

bool Company::ExecuteUpcomingEvent() {
	Event* tempEvent = nullptr;
	this->EventList->peek(tempEvent);
	// checking if there was actually an event to execute next
	if (tempEvent != nullptr) {
		// Total Hours of the Event & Timestep Relative to the Starting Point.
		int EventTotalHours = tempEvent->GetEventTime().GetTotalHours();
		int TimestepTotalHours = TimestepNum.GetTotalHours();
		// Executes the event in case the current time is equal to the event time.
		if (TimestepTotalHours >= EventTotalHours) {
			tempEvent->Execute();
			EventList->dequeue(tempEvent);
			delete tempEvent;
			return true;
		}
	}

	return false;
}

std::string Company::GetCurrentTime() {
	int hours = this->TimestepNum.GetHour();
	int day = this->TimestepNum.GetDay();

	return to_string(day) + ":" + to_string(hours);
}

bool Company::LoadVIPCargosToTruck()
{	

	if (this->VIPCargoList->getCount() != 0) {
		
		//checks first for the availability of VIP trucks
		Truck* vipTruck;
		this->VIPTrucksList->peek(vipTruck);
		
		if (vipTruck != nullptr) {
			if (this->VIPCargoList->getCount() < vipTruck->GetCapacity()) {
				return false;
			}
			else {
				this->LoadTruck(vipTruck, this->VIPCargoList);
				return true;
			}
		}

		//checks second for the availability of normal trucks
		Truck* normalTruck;
		this->NormalTrucksList->peek(normalTruck);
		if (normalTruck != nullptr) {
			if (this->VIPCargoList->getCount() < normalTruck->GetCapacity()) {
				return false;
			}
			else {
				this->LoadTruck(normalTruck, this->VIPCargoList);
				return true;
			}

		}

		//checks last for the availability of special trucks
		Truck* specialTruck;
		this->SpecialTrucksList->peek(specialTruck);
		if (specialTruck != nullptr) {
			if (this->VIPCargoList->getCount() < specialTruck->GetCapacity()) {
				return false;
			}
			else {
				this->LoadTruck(specialTruck, this->VIPCargoList);
				return true;
			}
		}
	}
	return false;
}

bool Company::LoadSpecialCargosToTruck()
{
	Truck* specialTruck;
	this->SpecialTrucksList->peek(specialTruck);

	if (specialTruck != nullptr) {
		if (this->SpecialCargoList->getCount() < specialTruck->GetCapacity()) {
			return false;
		}
		else {
			this->LoadTruck(specialTruck, this->SpecialCargoList);
			return true;
		}
	}

	return false;
}

bool Company::LoadNormalCargosToTruck()
{
	Truck* normalTruck;
	this->NormalTrucksList->peek(normalTruck);
	if (normalTruck != nullptr) {
		if (this->NormalCargoList->getCount() < normalTruck->GetCapacity()) {
			return false;
		}
		else {
			this->LoadTruck(normalTruck, this->NormalCargoList);
			return true;
		}
	}

	Truck* vipTruck;
	this->VIPTrucksList->peek(vipTruck);
	if (vipTruck != nullptr) {
		if (this->NormalCargoList->getCount() < vipTruck->GetCapacity()) {
			return false;
		}
		else {
			this->LoadTruck(vipTruck, this->NormalCargoList);
			return true;
		}
	}

	return false;
}

void Company::LoadTruck(Truck* truck, Queue<Cargo*>* cargoQueue)
{
	Cargo* c;
	cargoQueue->peek(c);
	truck->LoadCargo(c);
	while (cargoQueue->dequeue(c)) {
		cargoQueue->peek(c);
		if(!(truck->LoadCargo(c)))
			break;
	}
}

void Company::LoadTruck(Truck* truck, LinkedList<Cargo*>* cargoList)
{
	Cargo* c = nullptr;
	Node<Cargo*>* tempNode;
	while (cargoList->GetHead() != nullptr) {

		if (c != nullptr) {
			if (!(truck->LoadCargo(c)))
				break;
		}
		c = cargoList->GetHead()->getItem();
		tempNode = cargoList->GetHead();

		cargoList->SetHead(cargoList->GetHead()->getNext());
		cargoList->setCount(cargoList->getCount() - 1);
		delete tempNode;
	}
}


template <typename T>
void Company::cleanQueueInnerPointers(Queue<T*>* queue)
{
	T* tempPointer = nullptr;
	while (queue->dequeue(tempPointer))
	{
		delete tempPointer;
		tempPointer = nullptr;
	}
}

template <typename T>
void Company::cleanPriorityQueueInnerPointers(PQueue<T*>* pqueue)
{
	T* tempPointer = nullptr;
	while (pqueue->dequeue(tempPointer))
	{
		delete tempPointer;
		tempPointer = nullptr;
	}
}


void Company::checkForAutoPromote() {
	Node<Cargo*>* Head = this->NormalCargoList->GetHead();
	
	while (Head != nullptr) {
		Cargo* pCargo = Head->getItem();
		Time res = (pCargo->GetPrepTime() - this->TimestepNum);
		if (this->AutoPromotionLimit <= res) {
			AutoPromote(pCargo);
		}
		Head = Head->getNext();
	}

}

bool Company::isChangeableCargo(int ID) {
	return (this->FindNormalCargo(ID) != nullptr) ? true : false;
}

void Company::AutoPromote(Cargo* pCargo) {
	//if a cargo wait more than auotp days from its preparation time to be assigned to a truck,
	//it should be automatically promoted to be an vip cargo
		 
		this->DeleteNormalCargo(pCargo->GetID());
		this->AddVIPCargo(pCargo);
	
}

void Company::MoveTrucks() {
	Truck* checkingTruck = nullptr;

	this->NormalTrucksList->peek(checkingTruck);
	if (checkingTruck->IsLoaded()) {
		NormalTrucksList->dequeue(checkingTruck);
		MovingTrucks->enqueue(checkingTruck, checkingTruck->GetTruckPriority());
	}

	this->SpecialTrucksList->peek(checkingTruck);
	if (checkingTruck->IsLoaded()) {
		SpecialTrucksList->dequeue(checkingTruck);
		MovingTrucks->enqueue(checkingTruck, checkingTruck->GetTruckPriority());
	}

	this->VIPTrucksList->peek(checkingTruck);
	if (checkingTruck->IsLoaded()) {
		VIPTrucksList->dequeue(checkingTruck);
		MovingTrucks->enqueue(checkingTruck, checkingTruck->GetTruckPriority());
	}
}

void Company::CheckForCheckUp() {
	
	// check for normal trucks
	Truck* pTruck;
	while (true) {
		this->NormalTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;
		if (pTruck->GetJourneysBeforeCheckUp() == 0) {
			pTruck->ResetJourneysCount();
			Time OutTime = pTruck->GetCheckUpTime() + this->TimestepNum;
			if (this->CheckForMaintenance(pTruck)) {
				OutTime = OutTime + 10;
				pTruck->setCheckUpOutTime(OutTime);
				this->NormalMaintenanceTrucksList->enqueue(pTruck);
			}
			else {
				pTruck->setCheckUpOutTime(OutTime);
				this->InCheckUpTrucksList->enqueue(pTruck, -OutTime.GetTotalHours());
			}
			
			
			this->NormalTrucksList->dequeue(pTruck);
		}
		else break;
	}

	// check for special trucks
	while (true) {
		this->SpecialTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;
		if (pTruck->GetJourneysBeforeCheckUp() == 0) {
			pTruck->ResetJourneysCount();
			Time OutTime = pTruck->GetCheckUpTime() + this->TimestepNum;
			if (this->CheckForMaintenance(pTruck)) {
				OutTime = OutTime + 10;
				pTruck->setCheckUpOutTime(OutTime);
				this->SpecialMaintenanceTrucksList->enqueue(pTruck);
			}
			else {
				pTruck->setCheckUpOutTime(OutTime);
				this->InCheckUpTrucksList->enqueue(pTruck, -OutTime.GetTotalHours());
			}
			this->SpecialTrucksList->dequeue(pTruck);

		}
		else break;
	}

	// check for vip trucks
	while (true) {
		this->VIPTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;
		if (pTruck->GetJourneysBeforeCheckUp() == 0) {
			pTruck->ResetJourneysCount();
			Time OutTime = pTruck->GetCheckUpTime() + this->TimestepNum;
			if (this->CheckForMaintenance(pTruck)) {
				OutTime = OutTime + 10;
				pTruck->setCheckUpOutTime(OutTime);
				this->VIPMaintenanceTrucksList->enqueue(pTruck);
			}
			else {
				pTruck->setCheckUpOutTime(OutTime);
				this->InCheckUpTrucksList->enqueue(pTruck, -OutTime.GetTotalHours());
			}
			this->VIPTrucksList->dequeue(pTruck);
		}
		else break;
	}
}

void Company::MoveCheckUpToAvailable() {
	Truck* pTruck;
	while (true) {
		this->InCheckUpTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;

		if (pTruck->getCheckUpOutTime() <= this->TimestepNum) {
			
			switch (pTruck->GetTruckType())
			{
			case (TRUCKTYPE::NT):
				this->NormalTrucksList->enqueue(pTruck);
				break;
			case (TRUCKTYPE::ST):
				this->SpecialTrucksList->enqueue(pTruck);
				break;
			case (TRUCKTYPE::VT):
				this->VIPTrucksList->enqueue(pTruck);
				break;
			}

			this->InCheckUpTrucksList->dequeue(pTruck);
		}
		else break;
	}
}

bool Company::CheckForMaintenance(Truck* pTruck) {
	// maintenance factors ::
	// - tiers have completed a total number of trips
	// - total distance more than 10000 KM
	// - delivery time spent

	if (1 == 2) {
		return true;
	}
	else {
		return false;
	}
}

void Company::MoveMaintenanceToAvailable() {
	Truck* pTruck;

	// normal truck move to available
	while (true) {
		this->NormalMaintenanceTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;

		if (pTruck->getCheckUpOutTime() <= this->TimestepNum) {
			this->NormalTrucksList->enqueue(pTruck);
			this->NormalMaintenanceTrucksList->dequeue(pTruck);
		}
		else break;
	}

	// special truck move to available
	while (true) {
		this->SpecialMaintenanceTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;

		if (pTruck->getCheckUpOutTime() <= this->TimestepNum) {
			this->SpecialTrucksList->enqueue(pTruck);
			this->SpecialMaintenanceTrucksList->dequeue(pTruck);
		}
		else break;
	}

	// vip truck move to available
	while (true) {
		this->VIPMaintenanceTrucksList->peek(pTruck);
		if (pTruck == nullptr) break;

		if (pTruck->getCheckUpOutTime() <= this->TimestepNum) {
			this->VIPTrucksList->enqueue(pTruck);
			this->VIPMaintenanceTrucksList->dequeue(pTruck);
		}
		else break;
	}
}

#endif 