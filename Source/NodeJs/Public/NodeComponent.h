// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SocketIOClientComponent.h"
#include "NodeCmd.h"
#include "NodeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNodeSciptBeginSignature, int32, ProcessId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNodeConsoleLogSignature, FString, LogMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNodeScriptEndSignature, FString, FinishedScriptRelativePath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNodeScriptErrorSignature, FString, ScriptRelativePath, FString, ErrorMessage);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NODEJS_API UNodeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UPROPERTY(BlueprintAssignable, Category = "NodeJs Events")
	FSIOCEventJsonSignature OnEvent;

	UPROPERTY(BlueprintAssignable, Category = "NodeJs Events")
	FNodeConsoleLogSignature OnConsoleLog;

	UPROPERTY(BlueprintAssignable, Category = "NodeJs Events")
	FNodeSciptBeginSignature OnScriptBegin;

	UPROPERTY(BlueprintAssignable, Category = "NodeJs Events")
	FNodeScriptEndSignature OnScriptEnd;

	UPROPERTY(BlueprintAssignable, Category = "NodeJs Events")
	FNodeScriptErrorSignature OnScriptError;

	//set this to true if you'd like the default script to start with the component
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NodeJsProperties)
	bool bRunDefaultScriptOnBeginPlay;

	//not yet implemented
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NodeJsProperties)
	bool bReloadOnChange;

	//This should always be true, removed from BP exposure
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NodeJsProperties)
	bool bStartMainScriptIfNeededOnBeginPlay;

	//This will cleanup our main script thread whenever there are no listeners. May slow down quick map travels. Default off.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NodeJsProperties)
	bool bStopMainScriptOnNoListeners;

	//Relative to {project root}/Content/Scripts
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = NodeJsProperties)
	FString DefaultScriptPath;

	//we only allow one script per component
	UPROPERTY(BlueprintReadOnly, Category = NodeJsProperties)
	bool bScriptIsRunning;

	//-1 if invalid
	UPROPERTY(BlueprintReadOnly, Category = NodeJsProperties)
	int32 ScriptId;

	/** Run your scripts here */
	UFUNCTION(BlueprintCallable, Category = "NodeJs Functions")
	void RunScript(const FString& ScriptRelativePath);

	/** forcibly stop the script */
	UFUNCTION(BlueprintCallable, Category = "NodeJs Functions")
	void StopScript();

	/**
	* Emit an event with a JsonValue message
	*
	* @param Name		Event name
	* @param Message	SIOJJsonValue
	* @param Namespace	Namespace within socket.io
	*/
	UFUNCTION(BlueprintCallable, Category = "NodeJs Functions")
	void Emit(const FString& EventName, USIOJsonValue* Message = nullptr, const FString& Namespace = FString(TEXT("/")));

	/**
	* Emit an event with a JsonValue message with a callback function defined by CallBackFunctionName
	* This may not work due to ipc-event-emitter support
	*
	* @param Name					Event name
	* @param Message				SIOJsonValue
	* @param CallbackFunctionName	Name of the optional callback function with signature (String, SIOJsonValue)
	* @param Target					CallbackFunction target object, typically self where this is called.
	* @param Namespace				Namespace within socket.io
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketIO Functions")
	void EmitWithCallBack(const FString& EventName,
			USIOJsonValue* Message = nullptr,
			const FString& CallbackFunctionName = FString(""),
			UObject* Target = nullptr,
			const FString& Namespace = FString(TEXT("/")));

	/**
	* Bind an event, then respond to it with 'On' multi-cast delegate
	*
	* @param EventName	Event name
	* @param Namespace	Optional namespace, defaults to default namespace
	*/
	UFUNCTION(BlueprintCallable, Category = "NodeJs Functions")
	void BindEvent(const FString& EventName, const FString& Namespace = FString(TEXT("/")));

	UNodeComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



protected:
	void LinkAndStartWrapperScript();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool CallBPFunctionWithResponse(UObject* Target, const FString& FunctionName, TArray<TSharedPtr<FJsonValue>> Response);

private:
	TSharedPtr<FNodeCmd> Cmd;
	TSharedPtr<FNodeEventListener> Listener;
	TArray<TFunction<void()>> DelayedBindEvents;
	TArray<FString> BoundEventNames;

	//append process id for mux routing in main script
	FString FullEventName(const FString& EventName);
	void UnbindAllScriptEvents();
};
