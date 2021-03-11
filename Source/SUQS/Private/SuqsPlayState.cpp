#include "SuqsPlayState.h"
#include "SuqsObjectiveState.h"
#include "SuqsQuestState.h"
#include "SuqsTaskState.h"


DEFINE_LOG_CATEGORY(LogSuqsPlayState)

void USuqsPlayState::EnsureStateBuilt()
{
	EnsureQuestDefinitionsBuilt();
}


void USuqsPlayState::EnsureQuestDefinitionsBuilt()
{
	// Build unified quest table
	if (QuestDefinitions.Num() == 0 && QuestDataTables.Num() > 0)
	{
		for (auto Table : QuestDataTables)
		{
			Table->ForeachRow<FSuqsQuest>("", [this, Table](const FName& Key, const FSuqsQuest& Quest)
            {
                if (QuestDefinitions.Contains(Key))
                	UE_LOG(LogSuqsPlayState, Error, TEXT("Quest name '%s' has been used more than once! Duplicate entry was in %s"), *Key.ToString(), *Table->GetName());

                // Check task IDs are unique
                TSet<FName> TaskIDSet;
                for (auto& Objective : Quest.Objectives)
                {
                    for (auto& Task : Objective.Tasks)
                    {
                        bool bDuplicate;
                        TaskIDSet.Add(Task.Identifier, &bDuplicate);
                        if (bDuplicate)
                        	UE_LOG(LogSuqsPlayState, Error, TEXT("Task ID '%s' has been used more than once! Duplicate entry title: %s"), *Task.Identifier.ToString(), *Task.Title.ToString());
                    }
                }
				
                QuestDefinitions.Add(Key, Quest);
            });
		}
	}
}

ESuqsQuestStatus USuqsPlayState::GetQuestStatus(FName QuestID) const
{
	// Could make a lookup for this, but we'd need to post-load call to re-populate it, leave for now
	const auto State = FindQuestStatus(QuestID);

	if (State)
		return State->GetStatus();
	else
		return ESuqsQuestStatus::Unavailable;
	
}

USuqsQuestState* USuqsPlayState::GetQuest(FName QuestID)
{
	auto Status = FindQuestStatus(QuestID);
	return Status;
}


USuqsQuestState* USuqsPlayState::FindQuestStatus(const FName& QuestID)
{
	auto PQ = ActiveQuests.Find(QuestID);
	if (PQ)
		return *PQ;
	PQ = QuestArchive.Find(QuestID);
	if (PQ)
		return *PQ;

	UE_LOG(LogSuqsPlayState, Error, TEXT("Requested non-existent quest %s"), *QuestID.ToString());
	return nullptr;
	
}

const USuqsQuestState* USuqsPlayState::FindQuestStatus(const FName& QuestID) const
{
	return const_cast<USuqsPlayState*>(this)->FindQuestStatus(QuestID);
}

USuqsTaskState* USuqsPlayState::FindTaskStatus(const FName& QuestID, const FName& TaskID)
{
	auto Q = FindQuestStatus(QuestID);
	if (Q)
	{
		return Q->FindTask(TaskID);
	}
	
	return nullptr;
}


void USuqsPlayState::GetAcceptedQuestIdentifiers(TArray<FName>& AcceptedQuestIDsOut) const
{
	ActiveQuests.GenerateKeyArray(AcceptedQuestIDsOut);
}

void USuqsPlayState::GetArchivedQuestIdentifiers(TArray<FName>& ArchivedQuestIDsOut) const
{
	QuestArchive.GenerateKeyArray(ArchivedQuestIDsOut);
}


void USuqsPlayState::GetAcceptedQuests(TArray<USuqsQuestState*>& AcceptedQuestsOut) const
{
	ActiveQuests.GenerateValueArray(AcceptedQuestsOut);
}

void USuqsPlayState::GetArchivedQuests(TArray<USuqsQuestState*>& ArchivedQuestsOut) const
{
	QuestArchive.GenerateValueArray(ArchivedQuestsOut);
}

bool USuqsPlayState::AcceptQuest(FName QuestID, bool bResetIfComplete, bool bResetIfInProgress)
{
	auto QDef = QuestDefinitions.Find(QuestID);
	if (QDef)
	{
		// Check that we don't already have this quest
		// TODO
		return true;
		
		//OnQuestAccepted.Broadcast(Quest);
	}
	else
	{
		UE_LOG(LogSuqsPlayState, Error , TEXT("Attempted to accept a non-existent quest %s"), *QuestID.ToString());
		return false;
	}
	
}

void USuqsPlayState::FailQuest(FName QuestID)
{
	auto Q = FindQuestStatus(QuestID);
	if (Q)
		Q->Fail();
}

void USuqsPlayState::FailTask(FName QuestID, FName TaskIdentifier)
{
	auto T = FindTaskStatus(QuestID, TaskIdentifier);
	if (T)
	{
		T->Fail();
	}
}


void USuqsPlayState::CompleteTask(FName QuestID, FName TaskIdentifier)
{
	auto T = FindTaskStatus(QuestID, TaskIdentifier);
	if (T)
	{
		T->Complete();
	}
}

void USuqsPlayState::ProgressTask(FName QuestID, FName TaskIdentifier, int Delta)
{
	auto T = FindTaskStatus(QuestID, TaskIdentifier);
	if (T)
	{
		T->Progress(Delta);
	}
}


void USuqsPlayState::RaiseQuestUpdated(USuqsQuestState* Quest)
{
	// might be worth queuing these up and raising combined, since one change can
	// trigger this from e.g. task, objective and quest all changing as a cascade
	OnQuestUpdated.Broadcast(Quest);
}

void USuqsPlayState::RaiseTaskCompleted(USuqsTaskState* Task)
{
	OnTaskCompleted.Broadcast(Task);
}
void USuqsPlayState::RaiseTaskFailed(USuqsTaskState* Task)
{
	OnTaskFailed.Broadcast(Task);
}


void USuqsPlayState::RaiseObjectiveCompleted(USuqsObjectiveState* Objective)
{
	OnObjectiveCompleted.Broadcast(Objective);
}

void USuqsPlayState::RaiseObjectiveFailed(USuqsObjectiveState* Objective)
{
	OnObjectiveFailed.Broadcast(Objective);
}


void USuqsPlayState::RaiseQuestCompleted(USuqsQuestState* Quest)
{
	OnQuestCompleted.Broadcast(Quest);

	// TODO: trigger the acceptance of quests which depend on this completion
}

void USuqsPlayState::RaiseQuestFailed(USuqsQuestState* Quest)
{
	OnQuestFailed.Broadcast(Quest);

	// TODO: trigger the acceptance of quests which depend on this failure
}

// FTickableGameObject start
void USuqsPlayState::Tick(float DeltaTime)
{
	for (auto& QuestPair : ActiveQuests)
	{
		QuestPair.Value->Tick(DeltaTime);
	}
}

TStatId USuqsPlayState::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USuqsStatus, STATGROUP_Tickables);
}
// FTickableGameObject end
