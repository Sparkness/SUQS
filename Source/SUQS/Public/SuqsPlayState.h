#pragma once

#include "CoreMinimal.h"
#include "SuqsQuest.h"
#include "SuqsQuestState.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "SuqsPlayState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSuqsPlayState, Verbose, Verbose);

/**
 * Holder for all the state relating to quests and their objectives/tasks for a single player.
 * Add this somewhere that's useful to you, e.g. your PlayerState or GameInstance.
 */
UCLASS(BlueprintType)
class SUQS_API USuqsPlayState : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/// Provide one or more data assets which define the quests that this status is tracking against.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quest Setup")
	TArray<UDataTable*> QuestDataTables;

protected:
	/// Unified quest defs, combined from all entries in QuestDataTables
	UPROPERTY()
	TMap<FName, FSuqsQuest> QuestDefinitions;
	
	UPROPERTY()
	TMap<FName, USuqsQuestState*> QuestState;

	USuqsQuestState* FindQuestStatus(const FName& QuestName);
	const USuqsQuestState* FindQuestStatus(const FName& QuestName) const;
	USuqsTaskState* FindTaskStatus(const FName& QuestName, const FName& TaskID);

	void EnsureStateBuilt();
	void EnsureQuestDefinitionsBuilt();
	
public:

	/// Get the overall status of a named quest
	UFUNCTION(BlueprintCallable)
	ESuqsQuestStatus GetQuestState(const FName& Name) const;
	
	/// Return whether the named quest is or has been accepted for the player (may also be completed / failed)
	UFUNCTION(BlueprintCallable)
    bool IsQuestAccepted(const FName& Name) const { return GetQuestState(Name) != ESuqsQuestStatus::Unavailable; }

	/// Return whether the named quest is completed
	UFUNCTION(BlueprintCallable)
	bool IsQuestCompleted(const FName& Name) const { return GetQuestState(Name) == ESuqsQuestStatus::Completed; }

	/// Return whether the named quest has failed
	UFUNCTION(BlueprintCallable)
    bool IsQuestFailed(const FName& Name) const { return GetQuestState(Name) == ESuqsQuestStatus::Completed; }

	/// Accept a quest and track its state
	/// Note: you don't need to do this for quests which are set to auto-activate based on the completion of other quests.
	/// However you will want to do it for events that you activate other ways, e.g. entering areas, talking to characters
	UFUNCTION(BlueprintCallable)
	void AcceptQuest(const FName& Name);

	/// Manually fail a quest. You should prefer using FailTask() instead if you need to explain which specific part
	/// of a quest failed. Otherwise, this will mark all uncompleted tasks /objectives as failed.
	UFUNCTION(BlueprintCallable)
    void FailQuest(const FName& Name);

	/**
	 * Mark a task as failed. If this is a mandatory task, it will fail the objective the task is attached to.
	   If the objective is mandatory, it will fail the quest. 
	 * @param QuestName The name of the quest
	 * @param TaskIdentifier The identifier of the task within the quest
	 */
	UFUNCTION(BlueprintCallable)
    void FailTask(const FName& QuestName, const FName& TaskIdentifier);

	/**
	 * Fully complete a task. If this is the last mandatory task in an objective, also completes the objective, and
	 * cascades upwards to the quest if that's the last mandatory objective.
	 * @param QuestName The name of the quest
	 * @param TaskIdentifier The identifier of the task within the quest
	 */
	UFUNCTION(BlueprintCallable)
	void CompleteTask(const FName& QuestName, const FName& TaskIdentifier);

	/**
	 * Increment task progress. Increases the number value on a task, clamping it to the min/max numbers in the quest
	 * definition. If this increment takes the task number to the target, it completes the task as per CompleteTask.
	 * @param QuestName The name of the quest
	 * @param TaskIdentifier The identifier of the task within the quest
	 * @param Delta The change to make to the number on the task
	 */
	UFUNCTION(BlueprintCallable)
	void ProgressTask(const FName& QuestName, const FName& TaskIdentifier, int Delta);

	/**
	 * Set whether a task is hidden. Tasks start life hidden or not according to the quest definition, this changes that.
	 * Hidden tasks are ignored as if they don't exist and changing this will cause the objective/task status to be re-evaluated.
	 * For sequential objectives / tasks this could cause some jumping around as things appear/disappear, allowing
	 * some branching / dynamism within quests.
	 * @param QuestName The name of the quest
	 * @param TaskIdentifier The identifier of the task within the quest
	 * @param bHidden Whether the task is hidden. If all tasks in an objective are hidden, the objective is hidden too.
	 */
	UFUNCTION(BlueprintCallable)
	void SetTaskHidden(const FName& QuestName, const FName& TaskIdentifier, bool bHidden);

	// FTickableGameObject begin
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject end
};
