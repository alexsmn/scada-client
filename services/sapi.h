// Created by Microsoft (R) C/C++ Compiler Version 14.00.50727.363 (ec0d5a39).
//
// c:\work\tests\speak\debug\sapi.tlh
//
// C++ source equivalent of Win32 type library C866CA3A-32F7-11D2-9602-00C04F8EE628
// compiler-generated file created 05/24/08 at 17:46:06 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace SpeechLib {

//
// Forward references and typedefs
//

struct __declspec(uuid("c866ca3a-32f7-11d2-9602-00c04f8ee628"))
/* LIBID */ __SpeechLib;
struct __declspec(uuid("ce17c09b-4efa-44d5-a4c9-59d9585ab0cd"))
/* dual interface */ ISpeechDataKey;
struct __declspec(uuid("c74a3adc-b727-4500-a84a-b526721c8b8c"))
/* dual interface */ ISpeechObjectToken;
struct __declspec(uuid("ca7eac50-2d01-4145-86d4-5ae7d70f4469"))
/* dual interface */ ISpeechObjectTokenCategory;
enum SpeechDataKeyLocation;
struct __declspec(uuid("9285b776-2e7b-4bc0-b53e-580eb6fa967f"))
/* dual interface */ ISpeechObjectTokens;
enum SpeechTokenContext;
enum SpeechTokenShellFolder;
struct __declspec(uuid("11b103d8-1142-4edf-a093-82fb3915f8cc"))
/* dual interface */ ISpeechAudioBufferInfo;
struct __declspec(uuid("c62d9c91-7458-47f6-862d-1ef86fb0b278"))
/* dual interface */ ISpeechAudioStatus;
enum SpeechAudioState;
struct __declspec(uuid("e6e9c590-3e18-40e3-8299-061f98bde7c7"))
/* dual interface */ ISpeechAudioFormat;
enum SpeechAudioFormatType;
struct __declspec(uuid("7a1ef0d5-1581-4741-88e4-209a49f11a10"))
/* dual interface */ ISpeechWaveFormatEx;
struct __declspec(uuid("6450336f-7d49-4ced-8097-49d6dee37294"))
/* dual interface */ ISpeechBaseStream;
enum SpeechStreamSeekPositionType;
struct __declspec(uuid("af67f125-ab39-4e93-b4a2-cc2e66e182a7"))
/* dual interface */ ISpeechFileStream;
enum SpeechStreamFileMode;
struct __declspec(uuid("eeb14b68-808b-4abe-a5ea-b51da7588008"))
/* dual interface */ ISpeechMemoryStream;
struct __declspec(uuid("1a9e9f4f-104f-4db8-a115-efd7fd0c97ae"))
/* dual interface */ ISpeechCustomStream;
struct __declspec(uuid("cff8e175-019e-11d3-a08e-00c04f8ef9b5"))
/* dual interface */ ISpeechAudio;
struct __declspec(uuid("3c76af6d-1fd7-4831-81d1-3b71d5a13c44"))
/* dual interface */ ISpeechMMSysAudio;
struct __declspec(uuid("269316d8-57bd-11d2-9eee-00c04f797396"))
/* dual interface */ ISpeechVoice;
struct __declspec(uuid("8be47b07-57f6-11d2-9eee-00c04f797396"))
/* dual interface */ ISpeechVoiceStatus;
enum SpeechRunState;
enum SpeechVoiceEvents;
enum SpeechVoicePriority;
enum SpeechVoiceSpeakFlags;
struct __declspec(uuid("a372acd1-3bef-4bbd-8ffb-cb3e2b416af8"))
/* dispinterface */ _ISpeechVoiceEvents;
enum SpeechVisemeFeature;
enum SpeechVisemeType;
struct __declspec(uuid("2d5f1c0c-bd75-4b08-9478-3b11fea2586c"))
/* dual interface */ ISpeechRecognizer;
enum SpeechRecognizerState;
struct __declspec(uuid("bff9e781-53ec-484e-bb8a-0e1b5551e35c"))
/* dual interface */ ISpeechRecognizerStatus;
struct __declspec(uuid("580aa49d-7e1e-4809-b8e2-57da806104b8"))
/* dual interface */ ISpeechRecoContext;
enum SpeechInterference;
enum SpeechRecoEvents;
enum SpeechRecoContextState;
enum SpeechRetainedAudioOptions;
struct __declspec(uuid("b6d6f79f-2158-4e50-b5bc-9a9ccd852a09"))
/* dual interface */ ISpeechRecoGrammar;
enum SpeechGrammarState;
struct __declspec(uuid("6ffa3b44-fc2d-40d1-8afc-32911c7f1ad1"))
/* dual interface */ ISpeechGrammarRules;
struct __declspec(uuid("afe719cf-5dd1-44f2-999c-7a399f1cfccc"))
/* dual interface */ ISpeechGrammarRule;
enum SpeechRuleAttributes;
struct __declspec(uuid("d4286f2c-ee67-45ae-b928-28d695362eda"))
/* dual interface */ ISpeechGrammarRuleState;
struct __declspec(uuid("eabce657-75bc-44a2-aa7f-c56476742963"))
/* dual interface */ ISpeechGrammarRuleStateTransitions;
struct __declspec(uuid("cafd1db1-41d1-4a06-9863-e2e81da17a9a"))
/* dual interface */ ISpeechGrammarRuleStateTransition;
enum SpeechGrammarRuleStateTransitionType;
enum SpeechGrammarWordType;
enum SpeechSpecialTransitionType;
enum SpeechLoadOption;
enum SpeechRuleState;
struct __declspec(uuid("3b9c7e7a-6eee-4ded-9092-11657279adbe"))
/* dual interface */ ISpeechTextSelectionInformation;
enum SpeechWordPronounceable;
struct __declspec(uuid("ed2879cf-ced9-4ee6-a534-de0191d5468d"))
/* dual interface */ ISpeechRecoResult;
struct __declspec(uuid("62b3b8fb-f6e7-41be-bdcb-056b1c29efc0"))
/* dual interface */ ISpeechRecoResultTimes;
struct __declspec(uuid("961559cf-4e67-4662-8bf0-d93f1fcd61b3"))
/* dual interface */ ISpeechPhraseInfo;
struct __declspec(uuid("a7bfe112-a4a0-48d9-b602-c313843f6964"))
/* dual interface */ ISpeechPhraseRule;
struct __declspec(uuid("9047d593-01dd-4b72-81a3-e4a0ca69f407"))
/* dual interface */ ISpeechPhraseRules;
enum SpeechEngineConfidence;
struct __declspec(uuid("08166b47-102e-4b23-a599-bdb98dbfd1f4"))
/* dual interface */ ISpeechPhraseProperties;
struct __declspec(uuid("ce563d48-961e-4732-a2e1-378a42b430be"))
/* dual interface */ ISpeechPhraseProperty;
struct __declspec(uuid("0626b328-3478-467d-a0b3-d0853b93dda3"))
/* dual interface */ ISpeechPhraseElements;
struct __declspec(uuid("e6176f96-e373-4801-b223-3b62c068c0b4"))
/* dual interface */ ISpeechPhraseElement;
enum SpeechDisplayAttributes;
struct __declspec(uuid("38bc662f-2257-4525-959e-2069d2596c05"))
/* dual interface */ ISpeechPhraseReplacements;
struct __declspec(uuid("2890a410-53a7-4fb5-94ec-06d4998e3d02"))
/* dual interface */ ISpeechPhraseReplacement;
struct __declspec(uuid("b238b6d5-f276-4c3d-a6c1-2974801c3cc2"))
/* dual interface */ ISpeechPhraseAlternates;
struct __declspec(uuid("27864a2a-2b9f-4cb8-92d3-0d2722fd1e73"))
/* dual interface */ ISpeechPhraseAlternate;
enum SpeechDiscardType;
enum SpeechBookmarkOptions;
enum SpeechFormatType;
struct __declspec(uuid("7b8fcb42-0e9d-4f00-a048-7b04d6179d3d"))
/* dispinterface */ _ISpeechRecoContextEvents;
enum SpeechRecognitionType;
struct __declspec(uuid("8e0a246d-d3c8-45de-8657-04290c458c3c"))
/* dual interface */ ISpeechRecoResult2;
struct __declspec(uuid("3da7627a-c7ae-4b23-8708-638c50362c25"))
/* dual interface */ ISpeechLexicon;
enum SpeechLexiconType;
struct __declspec(uuid("8d199862-415e-47d5-ac4f-faa608b424e6"))
/* dual interface */ ISpeechLexiconWords;
struct __declspec(uuid("4e5b933c-c9be-48ed-8842-1ee51bb1d4ff"))
/* dual interface */ ISpeechLexiconWord;
enum SpeechWordType;
struct __declspec(uuid("72829128-5682-4704-a0d4-3e2bb6f2ead3"))
/* dual interface */ ISpeechLexiconPronunciations;
struct __declspec(uuid("95252c5d-9e43-4f4a-9899-48ee73352f9f"))
/* dual interface */ ISpeechLexiconPronunciation;
enum SpeechPartOfSpeech;
enum DISPID_SpeechDataKey;
enum DISPID_SpeechObjectToken;
enum DISPID_SpeechObjectTokens;
enum DISPID_SpeechObjectTokenCategory;
enum DISPID_SpeechAudioFormat;
enum DISPID_SpeechBaseStream;
enum DISPID_SpeechAudio;
enum DISPID_SpeechMMSysAudio;
enum DISPID_SpeechFileStream;
enum DISPID_SpeechCustomStream;
enum DISPID_SpeechMemoryStream;
enum DISPID_SpeechAudioStatus;
enum DISPID_SpeechAudioBufferInfo;
enum DISPID_SpeechWaveFormatEx;
enum DISPID_SpeechVoice;
enum DISPID_SpeechVoiceStatus;
enum DISPID_SpeechVoiceEvent;
enum DISPID_SpeechRecognizer;
enum SpeechEmulationCompareFlags;
enum DISPID_SpeechRecognizerStatus;
enum DISPID_SpeechRecoContext;
enum DISPIDSPRG;
enum DISPID_SpeechRecoContextEvents;
enum DISPID_SpeechGrammarRule;
enum DISPID_SpeechGrammarRules;
enum DISPID_SpeechGrammarRuleState;
enum DISPID_SpeechGrammarRuleStateTransitions;
enum DISPID_SpeechGrammarRuleStateTransition;
enum DISPIDSPTSI;
enum DISPID_SpeechRecoResult;
enum DISPID_SpeechXMLRecoResult;
struct __declspec(uuid("aaec54af-8f85-4924-944d-b79d39d72e19"))
/* dual interface */ ISpeechXMLRecoResult;
enum SPXMLRESULTOPTIONS;
enum DISPID_SpeechRecoResult2;
struct __declspec(uuid("6d60eb64-aced-40a6-bbf3-4e557f71dee2"))
/* dual interface */ ISpeechRecoResultDispatch;
enum DISPID_SpeechPhraseBuilder;
struct __declspec(uuid("3b151836-df3a-4e0a-846c-d2adc9334333"))
/* dual interface */ ISpeechPhraseInfoBuilder;
enum DISPID_SpeechRecoResultTimes;
enum DISPID_SpeechPhraseAlternate;
enum DISPID_SpeechPhraseAlternates;
enum DISPID_SpeechPhraseInfo;
enum DISPID_SpeechPhraseElement;
enum DISPID_SpeechPhraseElements;
enum DISPID_SpeechPhraseReplacement;
enum DISPID_SpeechPhraseReplacements;
enum DISPID_SpeechPhraseProperty;
enum DISPID_SpeechPhraseProperties;
enum DISPID_SpeechPhraseRule;
enum DISPID_SpeechPhraseRules;
enum DISPID_SpeechLexicon;
enum DISPID_SpeechLexiconWords;
enum DISPID_SpeechLexiconWord;
enum DISPID_SpeechLexiconProns;
enum DISPID_SpeechLexiconPronunciation;
enum DISPID_SpeechPhoneConverter;
struct __declspec(uuid("c3e4f353-433f-43d6-89a1-6a62a7054c3d"))
/* dual interface */ ISpeechPhoneConverter;
struct /* coclass */ SpNotifyTranslator;
struct __declspec(uuid("aca16614-5d3d-11d2-960e-00c04f8ee628"))
/* interface */ ISpNotifyTranslator;
struct __declspec(uuid("259684dc-37c3-11d2-9603-00c04f8ee628"))
/* interface */ ISpNotifySink;
struct /* coclass */ SpObjectTokenCategory;
struct __declspec(uuid("2d3d3845-39af-4850-bbf9-40b49780011d"))
/* interface */ ISpObjectTokenCategory;
struct __declspec(uuid("14056581-e16c-11d2-bb90-00c04f8ee6c0"))
/* interface */ ISpDataKey;
enum SPDATAKEYLOCATION;
struct __declspec(uuid("06b64f9e-7fda-11d2-b4f2-00c04f797396"))
/* interface */ IEnumSpObjectTokens;
struct __declspec(uuid("14056589-e16c-11d2-bb90-00c04f8ee6c0"))
/* interface */ ISpObjectToken;
struct /* coclass */ SpObjectToken;
struct /* coclass */ SpResourceManager;
struct __declspec(uuid("93384e18-5014-43d5-adbb-a78e055926bd"))
/* interface */ ISpResourceManager;
struct /* coclass */ SpStreamFormatConverter;
struct __declspec(uuid("678a932c-ea71-4446-9b41-78fda6280a29"))
/* interface */ ISpStreamFormatConverter;
struct __declspec(uuid("bed530be-2606-4f4d-a1c0-54c5cda5566f"))
/* interface */ ISpStreamFormat;
struct WaveFormatEx;
struct /* coclass */ SpMMAudioEnum;
struct /* coclass */ SpMMAudioIn;
struct __declspec(uuid("be7a9cce-5f9e-11d2-960f-00c04f8ee628"))
/* interface */ ISpEventSource;
struct __declspec(uuid("5eff4aef-8487-11d2-961c-00c04f8ee628"))
/* interface */ ISpNotifySource;
struct SPEVENT;
struct SPEVENTSOURCEINFO;
struct __declspec(uuid("be7a9cc9-5f9e-11d2-960f-00c04f8ee628"))
/* interface */ ISpEventSink;
struct __declspec(uuid("5b559f40-e952-11d2-bb91-00c04f8ee6c0"))
/* interface */ ISpObjectWithToken;
struct __declspec(uuid("15806f6e-1d70-4b48-98e6-3b1a007509ab"))
/* interface */ ISpMMSysAudio;
struct __declspec(uuid("c05c768f-fae8-4ec2-8e07-338321c12452"))
/* interface */ ISpAudio;
enum _SPAUDIOSTATE;
struct SPAUDIOSTATUS;
struct SPAUDIOBUFFERINFO;
struct /* coclass */ SpMMAudioOut;
struct /* coclass */ SpStream;
struct __declspec(uuid("12e3cca9-7518-44c5-a5e7-ba5a79cb929e"))
/* interface */ ISpStream;
enum SPFILEMODE;
struct /* coclass */ SpVoice;
struct __declspec(uuid("6c44df74-72b9-4992-a1ec-ef996e0422d4"))
/* interface */ ISpVoice;
struct SPVOICESTATUS;
enum SPVISEMES;
enum SPVPRIORITY;
enum SPEVENTENUM;
struct __declspec(uuid("b2745efd-42ce-48ca-81f1-a96e02538a90"))
/* interface */ ISpPhoneticAlphabetSelection;
struct /* coclass */ SpSharedRecoContext;
struct __declspec(uuid("f740a62f-7c15-489e-8234-940a33d9272d"))
/* interface */ ISpRecoContext;
struct __declspec(uuid("c2b5f241-daa0-4507-9e16-5a1eaa2b7a5c"))
/* interface */ ISpRecognizer;
struct __declspec(uuid("5b4fb971-b115-4de1-ad97-e482e3bf6ee4"))
/* interface */ ISpProperties;
enum SPRECOSTATE;
struct SPRECOGNIZERSTATUS;
enum SPWAVEFORMATTYPE;
struct __declspec(uuid("1a5c0354-b621-4b5a-8791-d306ed379e53"))
/* interface */ ISpPhrase;
struct SPPHRASE;
struct SPPHRASERULE;
struct SPPHRASEPROPERTY;
struct SPPHRASEELEMENT;
struct SPPHRASEREPLACEMENT;
struct SPSERIALIZEDPHRASE;
struct __declspec(uuid("2177db29-7f45-47d0-8554-067e91c80502"))
/* interface */ ISpRecoGrammar;
struct __declspec(uuid("8137828f-591a-4a42-be58-49ea7ebaac68"))
/* interface */ ISpGrammarBuilder;
enum SPGRAMMARWORDTYPE;
struct tagSPPROPERTYINFO;
enum SPLOADOPTIONS;
struct SPBINARYGRAMMAR;
enum SPRULESTATE;
struct tagSPTEXTSELECTIONINFO;
enum SPWORDPRONOUNCEABLE;
enum SPGRAMMARSTATE;
struct SPRECOCONTEXTSTATUS;
enum SPINTERFERENCE;
enum SPAUDIOOPTIONS;
struct SPSERIALIZEDRESULT;
struct __declspec(uuid("20b053be-e235-43cd-9a2a-8d17a48b7842"))
/* interface */ ISpRecoResult;
struct SPRECORESULTTIMES;
struct __declspec(uuid("8fcebc98-4e49-4067-9c6c-d86a0e092e3d"))
/* interface */ ISpPhraseAlt;
enum SPBOOKMARKOPTIONS;
enum SPCONTEXTSTATE;
struct __declspec(uuid("bead311c-52ff-437f-9464-6b21054ca73d"))
/* interface */ ISpRecoContext2;
enum SPADAPTATIONRELEVANCE;
struct /* coclass */ SpInprocRecognizer;
struct __declspec(uuid("8fc6d974-c81e-4098-93c5-0147f61ed4d3"))
/* interface */ ISpRecognizer2;
struct __declspec(uuid("21b501a0-0ec7-46c9-92c3-a2bc784c54b9"))
/* interface */ ISpSerializeState;
struct /* coclass */ SpSharedRecognizer;
struct /* coclass */ SpLexicon;
struct __declspec(uuid("da41a7c2-5383-4db2-916b-6c1719e3db58"))
/* interface */ ISpLexicon;
struct SPWORDPRONUNCIATIONLIST;
struct SPWORDPRONUNCIATION;
enum SPLEXICONTYPE;
enum SPPARTOFSPEECH;
struct SPWORDLIST;
struct SPWORD;
enum SPWORDTYPE;
struct /* coclass */ SpUnCompressedLexicon;
struct /* coclass */ SpCompressedLexicon;
struct /* coclass */ SpShortcut;
struct __declspec(uuid("3df681e2-ea56-11d9-8bde-f66bad1e3f3a"))
/* interface */ ISpShortcut;
enum SPSHORTCUTTYPE;
struct SPSHORTCUTPAIRLIST;
struct SPSHORTCUTPAIR;
struct /* coclass */ SpPhoneConverter;
struct __declspec(uuid("8445c581-0cac-4a38-abfe-9b2ce2826455"))
/* interface */ ISpPhoneConverter;
struct /* coclass */ SpPhoneticAlphabetConverter;
struct __declspec(uuid("133adcd4-19b4-4020-9fdc-842e78253b17"))
/* interface */ ISpPhoneticAlphabetConverter;
struct /* coclass */ SpNullPhoneConverter;
struct /* coclass */ SpTextSelectionInformation;
struct /* coclass */ SpPhraseInfoBuilder;
struct /* coclass */ SpAudioFormat;
struct /* coclass */ SpWaveFormatEx;
struct /* coclass */ SpInProcRecoContext;
struct /* coclass */ SpCustomStream;
struct /* coclass */ SpFileStream;
struct /* coclass */ SpMemoryStream;
struct __declspec(uuid("ae39362b-45a8-4074-9b9e-ccf49aa2d0b6"))
/* interface */ ISpXMLRecoResult;
struct SPSEMANTICERRORINFO;
struct __declspec(uuid("4b37bc9e-9ed6-44a3-93d3-18f022b79ec3"))
/* interface */ ISpRecoGrammar2;
struct SPRULE;
struct __declspec(uuid("b9ac5783-fcd0-4b21-b119-b4f8da8fd2c3"))
/* dual interface */ ISpeechResourceLoader;
struct __declspec(uuid("79eac9ee-baf9-11ce-8c82-00aa004ba90b"))
/* interface */ IInternetSecurityManager;
struct __declspec(uuid("79eac9ed-baf9-11ce-8c82-00aa004ba90b"))
/* interface */ IInternetSecurityMgrSite;
#if !defined(_WIN64)
typedef __w64 unsigned long UINT_PTR;
#else
typedef unsigned __int64 UINT_PTR;
#endif
#if !defined(_WIN64)
typedef __w64 long LONG_PTR;
#else
typedef __int64 LONG_PTR;
#endif
typedef enum _SPAUDIOSTATE SPAUDIOSTATE;
typedef enum SPWAVEFORMATTYPE SPSTREAMFORMATTYPE;
typedef struct tagSPPROPERTYINFO SPPROPERTYINFO;
typedef struct tagSPTEXTSELECTIONINFO SPTEXTSELECTIONINFO;
#if !defined(_WIN64)
typedef __w64 unsigned long ULONG_PTR;
#else
typedef unsigned __int64 ULONG_PTR;
#endif

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(ISpeechDataKey, __uuidof(ISpeechDataKey));
_COM_SMARTPTR_TYPEDEF(ISpeechAudioBufferInfo, __uuidof(ISpeechAudioBufferInfo));
_COM_SMARTPTR_TYPEDEF(ISpeechAudioStatus, __uuidof(ISpeechAudioStatus));
_COM_SMARTPTR_TYPEDEF(ISpeechWaveFormatEx, __uuidof(ISpeechWaveFormatEx));
_COM_SMARTPTR_TYPEDEF(ISpeechAudioFormat, __uuidof(ISpeechAudioFormat));
_COM_SMARTPTR_TYPEDEF(ISpeechBaseStream, __uuidof(ISpeechBaseStream));
_COM_SMARTPTR_TYPEDEF(ISpeechFileStream, __uuidof(ISpeechFileStream));
_COM_SMARTPTR_TYPEDEF(ISpeechMemoryStream, __uuidof(ISpeechMemoryStream));
_COM_SMARTPTR_TYPEDEF(ISpeechCustomStream, __uuidof(ISpeechCustomStream));
_COM_SMARTPTR_TYPEDEF(ISpeechAudio, __uuidof(ISpeechAudio));
_COM_SMARTPTR_TYPEDEF(ISpeechMMSysAudio, __uuidof(ISpeechMMSysAudio));
_COM_SMARTPTR_TYPEDEF(ISpeechVoiceStatus, __uuidof(ISpeechVoiceStatus));
_COM_SMARTPTR_TYPEDEF(_ISpeechVoiceEvents, __uuidof(_ISpeechVoiceEvents));
_COM_SMARTPTR_TYPEDEF(ISpeechRecognizerStatus, __uuidof(ISpeechRecognizerStatus));
_COM_SMARTPTR_TYPEDEF(ISpeechTextSelectionInformation, __uuidof(ISpeechTextSelectionInformation));
_COM_SMARTPTR_TYPEDEF(ISpeechRecoResultTimes, __uuidof(ISpeechRecoResultTimes));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseElement, __uuidof(ISpeechPhraseElement));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseElements, __uuidof(ISpeechPhraseElements));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseReplacement, __uuidof(ISpeechPhraseReplacement));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseReplacements, __uuidof(ISpeechPhraseReplacements));
_COM_SMARTPTR_TYPEDEF(_ISpeechRecoContextEvents, __uuidof(_ISpeechRecoContextEvents));
_COM_SMARTPTR_TYPEDEF(ISpeechLexiconPronunciation, __uuidof(ISpeechLexiconPronunciation));
_COM_SMARTPTR_TYPEDEF(ISpeechLexiconPronunciations, __uuidof(ISpeechLexiconPronunciations));
_COM_SMARTPTR_TYPEDEF(ISpeechLexiconWord, __uuidof(ISpeechLexiconWord));
_COM_SMARTPTR_TYPEDEF(ISpeechLexiconWords, __uuidof(ISpeechLexiconWords));
_COM_SMARTPTR_TYPEDEF(ISpeechLexicon, __uuidof(ISpeechLexicon));
_COM_SMARTPTR_TYPEDEF(ISpeechPhoneConverter, __uuidof(ISpeechPhoneConverter));
_COM_SMARTPTR_TYPEDEF(ISpNotifySink, __uuidof(ISpNotifySink));
_COM_SMARTPTR_TYPEDEF(ISpNotifyTranslator, __uuidof(ISpNotifyTranslator));
_COM_SMARTPTR_TYPEDEF(ISpDataKey, __uuidof(ISpDataKey));
_COM_SMARTPTR_TYPEDEF(ISpResourceManager, __uuidof(ISpResourceManager));
_COM_SMARTPTR_TYPEDEF(ISpStreamFormat, __uuidof(ISpStreamFormat));
_COM_SMARTPTR_TYPEDEF(ISpStreamFormatConverter, __uuidof(ISpStreamFormatConverter));
_COM_SMARTPTR_TYPEDEF(ISpNotifySource, __uuidof(ISpNotifySource));
_COM_SMARTPTR_TYPEDEF(ISpEventSource, __uuidof(ISpEventSource));
_COM_SMARTPTR_TYPEDEF(ISpEventSink, __uuidof(ISpEventSink));
_COM_SMARTPTR_TYPEDEF(ISpAudio, __uuidof(ISpAudio));
_COM_SMARTPTR_TYPEDEF(ISpMMSysAudio, __uuidof(ISpMMSysAudio));
_COM_SMARTPTR_TYPEDEF(ISpStream, __uuidof(ISpStream));
_COM_SMARTPTR_TYPEDEF(ISpPhoneticAlphabetSelection, __uuidof(ISpPhoneticAlphabetSelection));
_COM_SMARTPTR_TYPEDEF(ISpProperties, __uuidof(ISpProperties));
_COM_SMARTPTR_TYPEDEF(ISpPhrase, __uuidof(ISpPhrase));
_COM_SMARTPTR_TYPEDEF(ISpGrammarBuilder, __uuidof(ISpGrammarBuilder));
_COM_SMARTPTR_TYPEDEF(ISpPhraseAlt, __uuidof(ISpPhraseAlt));
_COM_SMARTPTR_TYPEDEF(ISpRecoContext2, __uuidof(ISpRecoContext2));
_COM_SMARTPTR_TYPEDEF(ISpRecognizer2, __uuidof(ISpRecognizer2));
_COM_SMARTPTR_TYPEDEF(ISpSerializeState, __uuidof(ISpSerializeState));
_COM_SMARTPTR_TYPEDEF(ISpLexicon, __uuidof(ISpLexicon));
_COM_SMARTPTR_TYPEDEF(ISpShortcut, __uuidof(ISpShortcut));
_COM_SMARTPTR_TYPEDEF(ISpPhoneticAlphabetConverter, __uuidof(ISpPhoneticAlphabetConverter));
_COM_SMARTPTR_TYPEDEF(ISpeechResourceLoader, __uuidof(ISpeechResourceLoader));
_COM_SMARTPTR_TYPEDEF(IInternetSecurityMgrSite, __uuidof(IInternetSecurityMgrSite));
_COM_SMARTPTR_TYPEDEF(IInternetSecurityManager, __uuidof(IInternetSecurityManager));
_COM_SMARTPTR_TYPEDEF(ISpRecoGrammar2, __uuidof(ISpRecoGrammar2));
_COM_SMARTPTR_TYPEDEF(ISpeechObjectToken, __uuidof(ISpeechObjectToken));
_COM_SMARTPTR_TYPEDEF(ISpeechObjectTokens, __uuidof(ISpeechObjectTokens));
_COM_SMARTPTR_TYPEDEF(ISpeechObjectTokenCategory, __uuidof(ISpeechObjectTokenCategory));
_COM_SMARTPTR_TYPEDEF(ISpeechVoice, __uuidof(ISpeechVoice));
_COM_SMARTPTR_TYPEDEF(ISpeechRecognizer, __uuidof(ISpeechRecognizer));
_COM_SMARTPTR_TYPEDEF(ISpeechRecoContext, __uuidof(ISpeechRecoContext));
_COM_SMARTPTR_TYPEDEF(ISpeechRecoGrammar, __uuidof(ISpeechRecoGrammar));
_COM_SMARTPTR_TYPEDEF(ISpeechGrammarRules, __uuidof(ISpeechGrammarRules));
_COM_SMARTPTR_TYPEDEF(ISpeechGrammarRule, __uuidof(ISpeechGrammarRule));
_COM_SMARTPTR_TYPEDEF(ISpeechGrammarRuleState, __uuidof(ISpeechGrammarRuleState));
_COM_SMARTPTR_TYPEDEF(ISpeechGrammarRuleStateTransition, __uuidof(ISpeechGrammarRuleStateTransition));
_COM_SMARTPTR_TYPEDEF(ISpeechGrammarRuleStateTransitions, __uuidof(ISpeechGrammarRuleStateTransitions));
_COM_SMARTPTR_TYPEDEF(ISpeechRecoResult, __uuidof(ISpeechRecoResult));
_COM_SMARTPTR_TYPEDEF(ISpeechRecoResult2, __uuidof(ISpeechRecoResult2));
_COM_SMARTPTR_TYPEDEF(ISpeechXMLRecoResult, __uuidof(ISpeechXMLRecoResult));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseInfo, __uuidof(ISpeechPhraseInfo));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseAlternate, __uuidof(ISpeechPhraseAlternate));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseAlternates, __uuidof(ISpeechPhraseAlternates));
_COM_SMARTPTR_TYPEDEF(ISpeechRecoResultDispatch, __uuidof(ISpeechRecoResultDispatch));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseInfoBuilder, __uuidof(ISpeechPhraseInfoBuilder));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseRule, __uuidof(ISpeechPhraseRule));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseRules, __uuidof(ISpeechPhraseRules));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseProperties, __uuidof(ISpeechPhraseProperties));
_COM_SMARTPTR_TYPEDEF(ISpeechPhraseProperty, __uuidof(ISpeechPhraseProperty));
_COM_SMARTPTR_TYPEDEF(ISpObjectTokenCategory, __uuidof(ISpObjectTokenCategory));
_COM_SMARTPTR_TYPEDEF(ISpObjectToken, __uuidof(ISpObjectToken));
_COM_SMARTPTR_TYPEDEF(IEnumSpObjectTokens, __uuidof(IEnumSpObjectTokens));
_COM_SMARTPTR_TYPEDEF(ISpObjectWithToken, __uuidof(ISpObjectWithToken));
_COM_SMARTPTR_TYPEDEF(ISpVoice, __uuidof(ISpVoice));
_COM_SMARTPTR_TYPEDEF(ISpPhoneConverter, __uuidof(ISpPhoneConverter));
_COM_SMARTPTR_TYPEDEF(ISpRecoContext, __uuidof(ISpRecoContext));
_COM_SMARTPTR_TYPEDEF(ISpRecognizer, __uuidof(ISpRecognizer));
_COM_SMARTPTR_TYPEDEF(ISpRecoGrammar, __uuidof(ISpRecoGrammar));
_COM_SMARTPTR_TYPEDEF(ISpRecoResult, __uuidof(ISpRecoResult));
_COM_SMARTPTR_TYPEDEF(ISpXMLRecoResult, __uuidof(ISpXMLRecoResult));

//
// Type library items
//

struct __declspec(uuid("ce17c09b-4efa-44d5-a4c9-59d9585ab0cd"))
ISpeechDataKey : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetBinaryValue (
        /*[in]*/ BSTR ValueName,
        /*[in]*/ VARIANT Value ) = 0;
      virtual HRESULT __stdcall GetBinaryValue (
        /*[in]*/ BSTR ValueName,
        /*[out,retval]*/ VARIANT * Value ) = 0;
      virtual HRESULT __stdcall SetStringValue (
        /*[in]*/ BSTR ValueName,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall GetStringValue (
        /*[in]*/ BSTR ValueName,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall SetLongValue (
        /*[in]*/ BSTR ValueName,
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall GetLongValue (
        /*[in]*/ BSTR ValueName,
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall OpenKey (
        /*[in]*/ BSTR SubKeyName,
        /*[out,retval]*/ struct ISpeechDataKey * * SubKey ) = 0;
      virtual HRESULT __stdcall CreateKey (
        /*[in]*/ BSTR SubKeyName,
        /*[out,retval]*/ struct ISpeechDataKey * * SubKey ) = 0;
      virtual HRESULT __stdcall DeleteKey (
        /*[in]*/ BSTR SubKeyName ) = 0;
      virtual HRESULT __stdcall DeleteValue (
        /*[in]*/ BSTR ValueName ) = 0;
      virtual HRESULT __stdcall EnumKeys (
        /*[in]*/ long Index,
        /*[out,retval]*/ BSTR * SubKeyName ) = 0;
      virtual HRESULT __stdcall EnumValues (
        /*[in]*/ long Index,
        /*[out,retval]*/ BSTR * ValueName ) = 0;
};

enum SpeechDataKeyLocation
{
    SDKLDefaultLocation = 0,
    SDKLCurrentUser = 1,
    SDKLLocalMachine = 2,
    SDKLCurrentConfig = 5
};

enum SpeechTokenContext
{
    STCInprocServer = 1,
    STCInprocHandler = 2,
    STCLocalServer = 4,
    STCRemoteServer = 16,
    STCAll = 23
};

enum SpeechTokenShellFolder
{
    STSF_AppData = 26,
    STSF_LocalAppData = 28,
    STSF_CommonAppData = 35,
    STSF_FlagCreate = 32768
};

struct __declspec(uuid("11b103d8-1142-4edf-a093-82fb3915f8cc"))
ISpeechAudioBufferInfo : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_MinNotification (
        /*[out,retval]*/ long * MinNotification ) = 0;
      virtual HRESULT __stdcall put_MinNotification (
        /*[in]*/ long MinNotification ) = 0;
      virtual HRESULT __stdcall get_BufferSize (
        /*[out,retval]*/ long * BufferSize ) = 0;
      virtual HRESULT __stdcall put_BufferSize (
        /*[in]*/ long BufferSize ) = 0;
      virtual HRESULT __stdcall get_EventBias (
        /*[out,retval]*/ long * EventBias ) = 0;
      virtual HRESULT __stdcall put_EventBias (
        /*[in]*/ long EventBias ) = 0;
};

enum SpeechAudioState
{
    SASClosed = 0,
    SASStop = 1,
    SASPause = 2,
    SASRun = 3
};

struct __declspec(uuid("c62d9c91-7458-47f6-862d-1ef86fb0b278"))
ISpeechAudioStatus : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_FreeBufferSpace (
        /*[out,retval]*/ long * FreeBufferSpace ) = 0;
      virtual HRESULT __stdcall get_NonBlockingIO (
        /*[out,retval]*/ long * NonBlockingIO ) = 0;
      virtual HRESULT __stdcall get_State (
        /*[out,retval]*/ enum SpeechAudioState * State ) = 0;
      virtual HRESULT __stdcall get_CurrentSeekPosition (
        /*[out,retval]*/ VARIANT * CurrentSeekPosition ) = 0;
      virtual HRESULT __stdcall get_CurrentDevicePosition (
        /*[out,retval]*/ VARIANT * CurrentDevicePosition ) = 0;
};

enum SpeechAudioFormatType
{
    SAFTDefault = -1,
    SAFTNoAssignedFormat = 0,
    SAFTText = 1,
    SAFTNonStandardFormat = 2,
    SAFTExtendedAudioFormat = 3,
    SAFT8kHz8BitMono = 4,
    SAFT8kHz8BitStereo = 5,
    SAFT8kHz16BitMono = 6,
    SAFT8kHz16BitStereo = 7,
    SAFT11kHz8BitMono = 8,
    SAFT11kHz8BitStereo = 9,
    SAFT11kHz16BitMono = 10,
    SAFT11kHz16BitStereo = 11,
    SAFT12kHz8BitMono = 12,
    SAFT12kHz8BitStereo = 13,
    SAFT12kHz16BitMono = 14,
    SAFT12kHz16BitStereo = 15,
    SAFT16kHz8BitMono = 16,
    SAFT16kHz8BitStereo = 17,
    SAFT16kHz16BitMono = 18,
    SAFT16kHz16BitStereo = 19,
    SAFT22kHz8BitMono = 20,
    SAFT22kHz8BitStereo = 21,
    SAFT22kHz16BitMono = 22,
    SAFT22kHz16BitStereo = 23,
    SAFT24kHz8BitMono = 24,
    SAFT24kHz8BitStereo = 25,
    SAFT24kHz16BitMono = 26,
    SAFT24kHz16BitStereo = 27,
    SAFT32kHz8BitMono = 28,
    SAFT32kHz8BitStereo = 29,
    SAFT32kHz16BitMono = 30,
    SAFT32kHz16BitStereo = 31,
    SAFT44kHz8BitMono = 32,
    SAFT44kHz8BitStereo = 33,
    SAFT44kHz16BitMono = 34,
    SAFT44kHz16BitStereo = 35,
    SAFT48kHz8BitMono = 36,
    SAFT48kHz8BitStereo = 37,
    SAFT48kHz16BitMono = 38,
    SAFT48kHz16BitStereo = 39,
    SAFTTrueSpeech_8kHz1BitMono = 40,
    SAFTCCITT_ALaw_8kHzMono = 41,
    SAFTCCITT_ALaw_8kHzStereo = 42,
    SAFTCCITT_ALaw_11kHzMono = 43,
    SAFTCCITT_ALaw_11kHzStereo = 44,
    SAFTCCITT_ALaw_22kHzMono = 45,
    SAFTCCITT_ALaw_22kHzStereo = 46,
    SAFTCCITT_ALaw_44kHzMono = 47,
    SAFTCCITT_ALaw_44kHzStereo = 48,
    SAFTCCITT_uLaw_8kHzMono = 49,
    SAFTCCITT_uLaw_8kHzStereo = 50,
    SAFTCCITT_uLaw_11kHzMono = 51,
    SAFTCCITT_uLaw_11kHzStereo = 52,
    SAFTCCITT_uLaw_22kHzMono = 53,
    SAFTCCITT_uLaw_22kHzStereo = 54,
    SAFTCCITT_uLaw_44kHzMono = 55,
    SAFTCCITT_uLaw_44kHzStereo = 56,
    SAFTADPCM_8kHzMono = 57,
    SAFTADPCM_8kHzStereo = 58,
    SAFTADPCM_11kHzMono = 59,
    SAFTADPCM_11kHzStereo = 60,
    SAFTADPCM_22kHzMono = 61,
    SAFTADPCM_22kHzStereo = 62,
    SAFTADPCM_44kHzMono = 63,
    SAFTADPCM_44kHzStereo = 64,
    SAFTGSM610_8kHzMono = 65,
    SAFTGSM610_11kHzMono = 66,
    SAFTGSM610_22kHzMono = 67,
    SAFTGSM610_44kHzMono = 68
};

struct __declspec(uuid("7a1ef0d5-1581-4741-88e4-209a49f11a10"))
ISpeechWaveFormatEx : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_FormatTag (
        /*[out,retval]*/ short * FormatTag ) = 0;
      virtual HRESULT __stdcall put_FormatTag (
        /*[in]*/ short FormatTag ) = 0;
      virtual HRESULT __stdcall get_Channels (
        /*[out,retval]*/ short * Channels ) = 0;
      virtual HRESULT __stdcall put_Channels (
        /*[in]*/ short Channels ) = 0;
      virtual HRESULT __stdcall get_SamplesPerSec (
        /*[out,retval]*/ long * SamplesPerSec ) = 0;
      virtual HRESULT __stdcall put_SamplesPerSec (
        /*[in]*/ long SamplesPerSec ) = 0;
      virtual HRESULT __stdcall get_AvgBytesPerSec (
        /*[out,retval]*/ long * AvgBytesPerSec ) = 0;
      virtual HRESULT __stdcall put_AvgBytesPerSec (
        /*[in]*/ long AvgBytesPerSec ) = 0;
      virtual HRESULT __stdcall get_BlockAlign (
        /*[out,retval]*/ short * BlockAlign ) = 0;
      virtual HRESULT __stdcall put_BlockAlign (
        /*[in]*/ short BlockAlign ) = 0;
      virtual HRESULT __stdcall get_BitsPerSample (
        /*[out,retval]*/ short * BitsPerSample ) = 0;
      virtual HRESULT __stdcall put_BitsPerSample (
        /*[in]*/ short BitsPerSample ) = 0;
      virtual HRESULT __stdcall get_ExtraData (
        /*[out,retval]*/ VARIANT * ExtraData ) = 0;
      virtual HRESULT __stdcall put_ExtraData (
        /*[in]*/ VARIANT ExtraData ) = 0;
};

struct __declspec(uuid("e6e9c590-3e18-40e3-8299-061f98bde7c7"))
ISpeechAudioFormat : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Type (
        /*[out,retval]*/ enum SpeechAudioFormatType * AudioFormat ) = 0;
      virtual HRESULT __stdcall put_Type (
        /*[in]*/ enum SpeechAudioFormatType AudioFormat ) = 0;
      virtual HRESULT __stdcall get_Guid (
        /*[out,retval]*/ BSTR * Guid ) = 0;
      virtual HRESULT __stdcall put_Guid (
        /*[in]*/ BSTR Guid ) = 0;
      virtual HRESULT __stdcall GetWaveFormatEx (
        /*[out,retval]*/ struct ISpeechWaveFormatEx * * WaveFormatEx ) = 0;
      virtual HRESULT __stdcall SetWaveFormatEx (
        /*[in]*/ struct ISpeechWaveFormatEx * WaveFormatEx ) = 0;
};

enum SpeechStreamSeekPositionType
{
    SSSPTRelativeToStart = 0,
    SSSPTRelativeToCurrentPosition = 1,
    SSSPTRelativeToEnd = 2
};

struct __declspec(uuid("6450336f-7d49-4ced-8097-49d6dee37294"))
ISpeechBaseStream : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Format (
        /*[out,retval]*/ struct ISpeechAudioFormat * * AudioFormat ) = 0;
      virtual HRESULT __stdcall putref_Format (
        /*[in]*/ struct ISpeechAudioFormat * AudioFormat ) = 0;
      virtual HRESULT __stdcall Read (
        /*[out]*/ VARIANT * Buffer,
        /*[in]*/ long NumberOfBytes,
        /*[out,retval]*/ long * BytesRead ) = 0;
      virtual HRESULT __stdcall Write (
        /*[in]*/ VARIANT Buffer,
        /*[out,retval]*/ long * BytesWritten ) = 0;
      virtual HRESULT __stdcall Seek (
        /*[in]*/ VARIANT Position,
        /*[in]*/ enum SpeechStreamSeekPositionType Origin,
        /*[out,retval]*/ VARIANT * NewPosition ) = 0;
};

enum SpeechStreamFileMode
{
    SSFMOpenForRead = 0,
    SSFMOpenReadWrite = 1,
    SSFMCreate = 2,
    SSFMCreateForWrite = 3
};

struct __declspec(uuid("af67f125-ab39-4e93-b4a2-cc2e66e182a7"))
ISpeechFileStream : ISpeechBaseStream
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Open (
        /*[in]*/ BSTR FileName,
        /*[in]*/ enum SpeechStreamFileMode FileMode,
        /*[in]*/ VARIANT_BOOL DoEvents ) = 0;
      virtual HRESULT __stdcall Close ( ) = 0;
};

struct __declspec(uuid("eeb14b68-808b-4abe-a5ea-b51da7588008"))
ISpeechMemoryStream : ISpeechBaseStream
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetData (
        /*[in]*/ VARIANT Data ) = 0;
      virtual HRESULT __stdcall GetData (
        /*[out,retval]*/ VARIANT * pData ) = 0;
};

struct __declspec(uuid("1a9e9f4f-104f-4db8-a115-efd7fd0c97ae"))
ISpeechCustomStream : ISpeechBaseStream
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_BaseStream (
        /*[out,retval]*/ IUnknown * * ppUnkStream ) = 0;
      virtual HRESULT __stdcall putref_BaseStream (
        /*[in]*/ IUnknown * ppUnkStream ) = 0;
};

struct __declspec(uuid("cff8e175-019e-11d3-a08e-00c04f8ef9b5"))
ISpeechAudio : ISpeechBaseStream
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Status (
        /*[out,retval]*/ struct ISpeechAudioStatus * * Status ) = 0;
      virtual HRESULT __stdcall get_BufferInfo (
        /*[out,retval]*/ struct ISpeechAudioBufferInfo * * BufferInfo ) = 0;
      virtual HRESULT __stdcall get_DefaultFormat (
        /*[out,retval]*/ struct ISpeechAudioFormat * * StreamFormat ) = 0;
      virtual HRESULT __stdcall get_Volume (
        /*[out,retval]*/ long * Volume ) = 0;
      virtual HRESULT __stdcall put_Volume (
        /*[in]*/ long Volume ) = 0;
      virtual HRESULT __stdcall get_BufferNotifySize (
        /*[out,retval]*/ long * BufferNotifySize ) = 0;
      virtual HRESULT __stdcall put_BufferNotifySize (
        /*[in]*/ long BufferNotifySize ) = 0;
      virtual HRESULT __stdcall get_EventHandle (
        /*[out,retval]*/ long * EventHandle ) = 0;
      virtual HRESULT __stdcall SetState (
        /*[in]*/ enum SpeechAudioState State ) = 0;
};

struct __declspec(uuid("3c76af6d-1fd7-4831-81d1-3b71d5a13c44"))
ISpeechMMSysAudio : ISpeechAudio
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_DeviceId (
        /*[out,retval]*/ long * DeviceId ) = 0;
      virtual HRESULT __stdcall put_DeviceId (
        /*[in]*/ long DeviceId ) = 0;
      virtual HRESULT __stdcall get_LineId (
        /*[out,retval]*/ long * LineId ) = 0;
      virtual HRESULT __stdcall put_LineId (
        /*[in]*/ long LineId ) = 0;
      virtual HRESULT __stdcall get_MMHandle (
        /*[out,retval]*/ long * Handle ) = 0;
};

enum SpeechRunState
{
    SRSEDone = 1,
    SRSEIsSpeaking = 2
};

struct __declspec(uuid("8be47b07-57f6-11d2-9eee-00c04f797396"))
ISpeechVoiceStatus : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_CurrentStreamNumber (
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall get_LastStreamNumberQueued (
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall get_LastHResult (
        /*[out,retval]*/ long * HResult ) = 0;
      virtual HRESULT __stdcall get_RunningState (
        /*[out,retval]*/ enum SpeechRunState * State ) = 0;
      virtual HRESULT __stdcall get_InputWordPosition (
        /*[out,retval]*/ long * Position ) = 0;
      virtual HRESULT __stdcall get_InputWordLength (
        /*[out,retval]*/ long * Length ) = 0;
      virtual HRESULT __stdcall get_InputSentencePosition (
        /*[out,retval]*/ long * Position ) = 0;
      virtual HRESULT __stdcall get_InputSentenceLength (
        /*[out,retval]*/ long * Length ) = 0;
      virtual HRESULT __stdcall get_LastBookmark (
        /*[out,retval]*/ BSTR * Bookmark ) = 0;
      virtual HRESULT __stdcall get_LastBookmarkId (
        /*[out,retval]*/ long * BookmarkId ) = 0;
      virtual HRESULT __stdcall get_PhonemeId (
        /*[out,retval]*/ short * PhoneId ) = 0;
      virtual HRESULT __stdcall get_VisemeId (
        /*[out,retval]*/ short * VisemeId ) = 0;
};

enum SpeechVoiceEvents
{
    SVEStartInputStream = 2,
    SVEEndInputStream = 4,
    SVEVoiceChange = 8,
    SVEBookmark = 16,
    SVEWordBoundary = 32,
    SVEPhoneme = 64,
    SVESentenceBoundary = 128,
    SVEViseme = 256,
    SVEAudioLevel = 512,
    SVEPrivate = 32768,
    SVEAllEvents = 33790
};

enum SpeechVoicePriority
{
    SVPNormal = 0,
    SVPAlert = 1,
    SVPOver = 2
};

enum SpeechVoiceSpeakFlags
{
    SVSFDefault = 0,
    SVSFlagsAsync = 1,
    SVSFPurgeBeforeSpeak = 2,
    SVSFIsFilename = 4,
    SVSFIsXML = 8,
    SVSFIsNotXML = 16,
    SVSFPersistXML = 32,
    SVSFNLPSpeakPunc = 64,
    SVSFParseSapi = 128,
    SVSFParseSsml = 256,
    SVSFParseAutodetect = 0,
    SVSFNLPMask = 64,
    SVSFParseMask = 384,
    SVSFVoiceMask = 511,
    SVSFUnusedFlags = -512
};

struct __declspec(uuid("a372acd1-3bef-4bbd-8ffb-cb3e2b416af8"))
_ISpeechVoiceEvents : IDispatch
{};

enum SpeechVisemeFeature
{
    SVF_None = 0,
    SVF_Stressed = 1,
    SVF_Emphasis = 2
};

enum SpeechVisemeType
{
    SVP_0 = 0,
    SVP_1 = 1,
    SVP_2 = 2,
    SVP_3 = 3,
    SVP_4 = 4,
    SVP_5 = 5,
    SVP_6 = 6,
    SVP_7 = 7,
    SVP_8 = 8,
    SVP_9 = 9,
    SVP_10 = 10,
    SVP_11 = 11,
    SVP_12 = 12,
    SVP_13 = 13,
    SVP_14 = 14,
    SVP_15 = 15,
    SVP_16 = 16,
    SVP_17 = 17,
    SVP_18 = 18,
    SVP_19 = 19,
    SVP_20 = 20,
    SVP_21 = 21
};

enum SpeechRecognizerState
{
    SRSInactive = 0,
    SRSActive = 1,
    SRSActiveAlways = 2,
    SRSInactiveWithPurge = 3
};

struct __declspec(uuid("bff9e781-53ec-484e-bb8a-0e1b5551e35c"))
ISpeechRecognizerStatus : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_AudioStatus (
        /*[out,retval]*/ struct ISpeechAudioStatus * * AudioStatus ) = 0;
      virtual HRESULT __stdcall get_CurrentStreamPosition (
        /*[out,retval]*/ VARIANT * pCurrentStreamPos ) = 0;
      virtual HRESULT __stdcall get_CurrentStreamNumber (
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall get_NumberOfActiveRules (
        /*[out,retval]*/ long * NumberOfActiveRules ) = 0;
      virtual HRESULT __stdcall get_ClsidEngine (
        /*[out,retval]*/ BSTR * ClsidEngine ) = 0;
      virtual HRESULT __stdcall get_SupportedLanguages (
        /*[out,retval]*/ VARIANT * SupportedLanguages ) = 0;
};

enum SpeechInterference
{
    SINone = 0,
    SINoise = 1,
    SINoSignal = 2,
    SITooLoud = 3,
    SITooQuiet = 4,
    SITooFast = 5,
    SITooSlow = 6
};

enum SpeechRecoEvents
{
    SREStreamEnd = 1,
    SRESoundStart = 2,
    SRESoundEnd = 4,
    SREPhraseStart = 8,
    SRERecognition = 16,
    SREHypothesis = 32,
    SREBookmark = 64,
    SREPropertyNumChange = 128,
    SREPropertyStringChange = 256,
    SREFalseRecognition = 512,
    SREInterference = 1024,
    SRERequestUI = 2048,
    SREStateChange = 4096,
    SREAdaptation = 8192,
    SREStreamStart = 16384,
    SRERecoOtherContext = 32768,
    SREAudioLevel = 65536,
    SREPrivate = 262144,
    SREAllEvents = 393215
};

enum SpeechRecoContextState
{
    SRCS_Disabled = 0,
    SRCS_Enabled = 1
};

enum SpeechRetainedAudioOptions
{
    SRAONone = 0,
    SRAORetainAudio = 1
};

enum SpeechGrammarState
{
    SGSEnabled = 1,
    SGSDisabled = 0,
    SGSExclusive = 3
};

enum SpeechRuleAttributes
{
    SRATopLevel = 1,
    SRADefaultToActive = 2,
    SRAExport = 4,
    SRAImport = 8,
    SRAInterpreter = 16,
    SRADynamic = 32,
    SRARoot = 64
};

enum SpeechGrammarRuleStateTransitionType
{
    SGRSTTEpsilon = 0,
    SGRSTTWord = 1,
    SGRSTTRule = 2,
    SGRSTTDictation = 3,
    SGRSTTWildcard = 4,
    SGRSTTTextBuffer = 5
};

enum SpeechGrammarWordType
{
    SGDisplay = 0,
    SGLexical = 1,
    SGPronounciation = 2,
    SGLexicalNoSpecialChars = 3
};

enum SpeechSpecialTransitionType
{
    SSTTWildcard = 1,
    SSTTDictation = 2,
    SSTTTextBuffer = 3
};

enum SpeechLoadOption
{
    SLOStatic = 0,
    SLODynamic = 1
};

enum SpeechRuleState
{
    SGDSInactive = 0,
    SGDSActive = 1,
    SGDSActiveWithAutoPause = 3,
    SGDSActiveUserDelimited = 4
};

struct __declspec(uuid("3b9c7e7a-6eee-4ded-9092-11657279adbe"))
ISpeechTextSelectionInformation : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall put_ActiveOffset (
        /*[in]*/ long ActiveOffset ) = 0;
      virtual HRESULT __stdcall get_ActiveOffset (
        /*[out,retval]*/ long * ActiveOffset ) = 0;
      virtual HRESULT __stdcall put_ActiveLength (
        /*[in]*/ long ActiveLength ) = 0;
      virtual HRESULT __stdcall get_ActiveLength (
        /*[out,retval]*/ long * ActiveLength ) = 0;
      virtual HRESULT __stdcall put_SelectionOffset (
        /*[in]*/ long SelectionOffset ) = 0;
      virtual HRESULT __stdcall get_SelectionOffset (
        /*[out,retval]*/ long * SelectionOffset ) = 0;
      virtual HRESULT __stdcall put_SelectionLength (
        /*[in]*/ long SelectionLength ) = 0;
      virtual HRESULT __stdcall get_SelectionLength (
        /*[out,retval]*/ long * SelectionLength ) = 0;
};

enum SpeechWordPronounceable
{
    SWPUnknownWordUnpronounceable = 0,
    SWPUnknownWordPronounceable = 1,
    SWPKnownWordPronounceable = 2
};

struct __declspec(uuid("62b3b8fb-f6e7-41be-bdcb-056b1c29efc0"))
ISpeechRecoResultTimes : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_StreamTime (
        /*[out,retval]*/ VARIANT * Time ) = 0;
      virtual HRESULT __stdcall get_Length (
        /*[out,retval]*/ VARIANT * Length ) = 0;
      virtual HRESULT __stdcall get_TickCount (
        /*[out,retval]*/ long * TickCount ) = 0;
      virtual HRESULT __stdcall get_OffsetFromStart (
        /*[out,retval]*/ VARIANT * OffsetFromStart ) = 0;
};

enum SpeechEngineConfidence
{
    SECLowConfidence = -1,
    SECNormalConfidence = 0,
    SECHighConfidence = 1
};

enum SpeechDisplayAttributes
{
    SDA_No_Trailing_Space = 0,
    SDA_One_Trailing_Space = 2,
    SDA_Two_Trailing_Spaces = 4,
    SDA_Consume_Leading_Spaces = 8
};

struct __declspec(uuid("e6176f96-e373-4801-b223-3b62c068c0b4"))
ISpeechPhraseElement : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_AudioTimeOffset (
        /*[out,retval]*/ long * AudioTimeOffset ) = 0;
      virtual HRESULT __stdcall get_AudioSizeTime (
        /*[out,retval]*/ long * AudioSizeTime ) = 0;
      virtual HRESULT __stdcall get_AudioStreamOffset (
        /*[out,retval]*/ long * AudioStreamOffset ) = 0;
      virtual HRESULT __stdcall get_AudioSizeBytes (
        /*[out,retval]*/ long * AudioSizeBytes ) = 0;
      virtual HRESULT __stdcall get_RetainedStreamOffset (
        /*[out,retval]*/ long * RetainedStreamOffset ) = 0;
      virtual HRESULT __stdcall get_RetainedSizeBytes (
        /*[out,retval]*/ long * RetainedSizeBytes ) = 0;
      virtual HRESULT __stdcall get_DisplayText (
        /*[out,retval]*/ BSTR * DisplayText ) = 0;
      virtual HRESULT __stdcall get_LexicalForm (
        /*[out,retval]*/ BSTR * LexicalForm ) = 0;
      virtual HRESULT __stdcall get_Pronunciation (
        /*[out,retval]*/ VARIANT * Pronunciation ) = 0;
      virtual HRESULT __stdcall get_DisplayAttributes (
        /*[out,retval]*/ enum SpeechDisplayAttributes * DisplayAttributes ) = 0;
      virtual HRESULT __stdcall get_RequiredConfidence (
        /*[out,retval]*/ enum SpeechEngineConfidence * RequiredConfidence ) = 0;
      virtual HRESULT __stdcall get_ActualConfidence (
        /*[out,retval]*/ enum SpeechEngineConfidence * ActualConfidence ) = 0;
      virtual HRESULT __stdcall get_EngineConfidence (
        /*[out,retval]*/ float * EngineConfidence ) = 0;
};

struct __declspec(uuid("0626b328-3478-467d-a0b3-d0853b93dda3"))
ISpeechPhraseElements : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechPhraseElement * * Element ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("2890a410-53a7-4fb5-94ec-06d4998e3d02"))
ISpeechPhraseReplacement : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_DisplayAttributes (
        /*[out,retval]*/ enum SpeechDisplayAttributes * DisplayAttributes ) = 0;
      virtual HRESULT __stdcall get_Text (
        /*[out,retval]*/ BSTR * Text ) = 0;
      virtual HRESULT __stdcall get_FirstElement (
        /*[out,retval]*/ long * FirstElement ) = 0;
      virtual HRESULT __stdcall get_NumberOfElements (
        /*[out,retval]*/ long * NumberOfElements ) = 0;
};

struct __declspec(uuid("38bc662f-2257-4525-959e-2069d2596c05"))
ISpeechPhraseReplacements : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechPhraseReplacement * * Reps ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

enum SpeechDiscardType
{
    SDTProperty = 1,
    SDTReplacement = 2,
    SDTRule = 4,
    SDTDisplayText = 8,
    SDTLexicalForm = 16,
    SDTPronunciation = 32,
    SDTAudio = 64,
    SDTAlternates = 128,
    SDTAll = 255
};

enum SpeechBookmarkOptions
{
    SBONone = 0,
    SBOPause = 1
};

enum SpeechFormatType
{
    SFTInput = 0,
    SFTSREngine = 1
};

struct __declspec(uuid("7b8fcb42-0e9d-4f00-a048-7b04d6179d3d"))
_ISpeechRecoContextEvents : IDispatch
{};

enum SpeechRecognitionType
{
    SRTStandard = 0,
    SRTAutopause = 1,
    SRTEmulated = 2,
    SRTSMLTimeout = 4,
    SRTExtendableParse = 8,
    SRTReSent = 16
};

enum SpeechLexiconType
{
    SLTUser = 1,
    SLTApp = 2
};

enum SpeechWordType
{
    SWTAdded = 1,
    SWTDeleted = 2
};

enum SpeechPartOfSpeech
{
    SPSNotOverriden = -1,
    SPSUnknown = 0,
    SPSNoun = 4096,
    SPSVerb = 8192,
    SPSModifier = 12288,
    SPSFunction = 16384,
    SPSInterjection = 20480,
    SPSLMA = 28672,
    SPSSuppressWord = 61440
};

struct __declspec(uuid("95252c5d-9e43-4f4a-9899-48ee73352f9f"))
ISpeechLexiconPronunciation : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Type (
        /*[out,retval]*/ enum SpeechLexiconType * LexiconType ) = 0;
      virtual HRESULT __stdcall get_LangId (
        /*[out,retval]*/ long * LangId ) = 0;
      virtual HRESULT __stdcall get_PartOfSpeech (
        /*[out,retval]*/ enum SpeechPartOfSpeech * PartOfSpeech ) = 0;
      virtual HRESULT __stdcall get_PhoneIds (
        /*[out,retval]*/ VARIANT * PhoneIds ) = 0;
      virtual HRESULT __stdcall get_Symbolic (
        /*[out,retval]*/ BSTR * Symbolic ) = 0;
};

struct __declspec(uuid("72829128-5682-4704-a0d4-3e2bb6f2ead3"))
ISpeechLexiconPronunciations : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechLexiconPronunciation * * Pronunciation ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("4e5b933c-c9be-48ed-8842-1ee51bb1d4ff"))
ISpeechLexiconWord : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_LangId (
        /*[out,retval]*/ long * LangId ) = 0;
      virtual HRESULT __stdcall get_Type (
        /*[out,retval]*/ enum SpeechWordType * WordType ) = 0;
      virtual HRESULT __stdcall get_Word (
        /*[out,retval]*/ BSTR * Word ) = 0;
      virtual HRESULT __stdcall get_Pronunciations (
        /*[out,retval]*/ struct ISpeechLexiconPronunciations * * Pronunciations ) = 0;
};

struct __declspec(uuid("8d199862-415e-47d5-ac4f-faa608b424e6"))
ISpeechLexiconWords : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechLexiconWord * * Word ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("3da7627a-c7ae-4b23-8708-638c50362c25"))
ISpeechLexicon : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_GenerationId (
        /*[out,retval]*/ long * GenerationId ) = 0;
      virtual HRESULT __stdcall GetWords (
        /*[in]*/ enum SpeechLexiconType Flags,
        /*[out]*/ long * GenerationId,
        /*[out,retval]*/ struct ISpeechLexiconWords * * Words ) = 0;
      virtual HRESULT __stdcall AddPronunciation (
        /*[in]*/ BSTR bstrWord,
        /*[in]*/ long LangId,
        /*[in]*/ enum SpeechPartOfSpeech PartOfSpeech,
        /*[in]*/ BSTR bstrPronunciation ) = 0;
      virtual HRESULT __stdcall AddPronunciationByPhoneIds (
        /*[in]*/ BSTR bstrWord,
        /*[in]*/ long LangId,
        /*[in]*/ enum SpeechPartOfSpeech PartOfSpeech,
        /*[in]*/ VARIANT * PhoneIds ) = 0;
      virtual HRESULT __stdcall RemovePronunciation (
        /*[in]*/ BSTR bstrWord,
        /*[in]*/ long LangId,
        /*[in]*/ enum SpeechPartOfSpeech PartOfSpeech,
        /*[in]*/ BSTR bstrPronunciation ) = 0;
      virtual HRESULT __stdcall RemovePronunciationByPhoneIds (
        /*[in]*/ BSTR bstrWord,
        /*[in]*/ long LangId,
        /*[in]*/ enum SpeechPartOfSpeech PartOfSpeech,
        /*[in]*/ VARIANT * PhoneIds ) = 0;
      virtual HRESULT __stdcall GetPronunciations (
        /*[in]*/ BSTR bstrWord,
        /*[in]*/ long LangId,
        /*[in]*/ enum SpeechLexiconType TypeFlags,
        /*[out,retval]*/ struct ISpeechLexiconPronunciations * * ppPronunciations ) = 0;
      virtual HRESULT __stdcall GetGenerationChange (
        /*[in,out]*/ long * GenerationId,
        /*[out,retval]*/ struct ISpeechLexiconWords * * ppWords ) = 0;
};
    const BSTR SpeechRegistryUserRoot = (wchar_t*) L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Speech";
    const BSTR SpeechRegistryLocalMachineRoot = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech";
    const BSTR SpeechCategoryAudioOut = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\AudioOutput";
    const BSTR SpeechCategoryAudioIn = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\AudioInput";
    const BSTR SpeechCategoryVoices = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Voices";
    const BSTR SpeechCategoryRecognizers = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Recognizers";
    const BSTR SpeechCategoryAppLexicons = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\AppLexicons";
    const BSTR SpeechCategoryPhoneConverters = (wchar_t*) L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\PhoneConverters";
    const BSTR SpeechCategoryRecoProfiles = (wchar_t*) L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Speech\\RecoProfiles";
    const BSTR SpeechTokenIdUserLexicon = (wchar_t*) L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Speech\\CurrentUserLexicon";
    const BSTR SpeechTokenValueCLSID = (wchar_t*) L"CLSID";
    const BSTR SpeechTokenKeyFiles = (wchar_t*) L"Files";
    const BSTR SpeechTokenKeyUI = (wchar_t*) L"UI";
    const BSTR SpeechTokenKeyAttributes = (wchar_t*) L"Attributes";
    const BSTR SpeechVoiceCategoryTTSRate = (wchar_t*) L"DefaultTTSRate";
    const BSTR SpeechPropertyResourceUsage = (wchar_t*) L"ResourceUsage";
    const BSTR SpeechPropertyHighConfidenceThreshold = (wchar_t*) L"HighConfidenceThreshold";
    const BSTR SpeechPropertyNormalConfidenceThreshold = (wchar_t*) L"NormalConfidenceThreshold";
    const BSTR SpeechPropertyLowConfidenceThreshold = (wchar_t*) L"LowConfidenceThreshold";
    const BSTR SpeechPropertyResponseSpeed = (wchar_t*) L"ResponseSpeed";
    const BSTR SpeechPropertyComplexResponseSpeed = (wchar_t*) L"ComplexResponseSpeed";
    const BSTR SpeechPropertyAdaptationOn = (wchar_t*) L"AdaptationOn";
    const BSTR SpeechDictationTopicSpelling = (wchar_t*) L"Spelling";
    const BSTR SpeechGrammarTagWildcard = (wchar_t*) L"...";
    const BSTR SpeechGrammarTagDictation = (wchar_t*) L"*";
    const BSTR SpeechGrammarTagUnlimitedDictation = (wchar_t*) L"*+";
    const BSTR SpeechEngineProperties = (wchar_t*) L"EngineProperties";
    const BSTR SpeechAddRemoveWord = (wchar_t*) L"AddRemoveWord";
    const BSTR SpeechUserTraining = (wchar_t*) L"UserTraining";
    const BSTR SpeechMicTraining = (wchar_t*) L"MicTraining";
    const BSTR SpeechRecoProfileProperties = (wchar_t*) L"RecoProfileProperties";
    const BSTR SpeechAudioProperties = (wchar_t*) L"AudioProperties";
    const BSTR SpeechAudioVolume = (wchar_t*) L"AudioVolume";
    const BSTR SpeechVoiceSkipTypeSentence = (wchar_t*) L"Sentence";
    const BSTR SpeechAudioFormatGUIDWave = (wchar_t*) L"{C31ADBAE-527F-4ff5-A230-F62BB61FF70C}";
    const BSTR SpeechAudioFormatGUIDText = (wchar_t*) L"{7CEEF9F9-3D13-11d2-9EE7-00C04F797396}";
    const float Speech_Default_Weight = 1;
    const long Speech_Max_Word_Length = 128;
    const long Speech_Max_Pron_Length = 384;
    const long Speech_StreamPos_Asap = 0;
    const long Speech_StreamPos_RealTime = -1;
    const long SpeechAllElements = -1;

enum DISPID_SpeechDataKey
{
    DISPID_SDKSetBinaryValue = 1,
    DISPID_SDKGetBinaryValue = 2,
    DISPID_SDKSetStringValue = 3,
    DISPID_SDKGetStringValue = 4,
    DISPID_SDKSetLongValue = 5,
    DISPID_SDKGetlongValue = 6,
    DISPID_SDKOpenKey = 7,
    DISPID_SDKCreateKey = 8,
    DISPID_SDKDeleteKey = 9,
    DISPID_SDKDeleteValue = 10,
    DISPID_SDKEnumKeys = 11,
    DISPID_SDKEnumValues = 12
};

enum DISPID_SpeechObjectToken
{
    DISPID_SOTId = 1,
    DISPID_SOTDataKey = 2,
    DISPID_SOTCategory = 3,
    DISPID_SOTGetDescription = 4,
    DISPID_SOTSetId = 5,
    DISPID_SOTGetAttribute = 6,
    DISPID_SOTCreateInstance = 7,
    DISPID_SOTRemove = 8,
    DISPID_SOTGetStorageFileName = 9,
    DISPID_SOTRemoveStorageFileName = 10,
    DISPID_SOTIsUISupported = 11,
    DISPID_SOTDisplayUI = 12,
    DISPID_SOTMatchesAttributes = 13
};

enum DISPID_SpeechObjectTokens
{
    DISPID_SOTsCount = 1,
    DISPID_SOTsItem = 0,
    DISPID_SOTs_NewEnum = -4
};

enum DISPID_SpeechObjectTokenCategory
{
    DISPID_SOTCId = 1,
    DISPID_SOTCDefault = 2,
    DISPID_SOTCSetId = 3,
    DISPID_SOTCGetDataKey = 4,
    DISPID_SOTCEnumerateTokens = 5
};

enum DISPID_SpeechAudioFormat
{
    DISPID_SAFType = 1,
    DISPID_SAFGuid = 2,
    DISPID_SAFGetWaveFormatEx = 3,
    DISPID_SAFSetWaveFormatEx = 4
};

enum DISPID_SpeechBaseStream
{
    DISPID_SBSFormat = 1,
    DISPID_SBSRead = 2,
    DISPID_SBSWrite = 3,
    DISPID_SBSSeek = 4
};

enum DISPID_SpeechAudio
{
    DISPID_SAStatus = 200,
    DISPID_SABufferInfo = 201,
    DISPID_SADefaultFormat = 202,
    DISPID_SAVolume = 203,
    DISPID_SABufferNotifySize = 204,
    DISPID_SAEventHandle = 205,
    DISPID_SASetState = 206
};

enum DISPID_SpeechMMSysAudio
{
    DISPID_SMSADeviceId = 300,
    DISPID_SMSALineId = 301,
    DISPID_SMSAMMHandle = 302
};

enum DISPID_SpeechFileStream
{
    DISPID_SFSOpen = 100,
    DISPID_SFSClose = 101
};

enum DISPID_SpeechCustomStream
{
    DISPID_SCSBaseStream = 100
};

enum DISPID_SpeechMemoryStream
{
    DISPID_SMSSetData = 100,
    DISPID_SMSGetData = 101
};

enum DISPID_SpeechAudioStatus
{
    DISPID_SASFreeBufferSpace = 1,
    DISPID_SASNonBlockingIO = 2,
    DISPID_SASState = 3,
    DISPID_SASCurrentSeekPosition = 4,
    DISPID_SASCurrentDevicePosition = 5
};

enum DISPID_SpeechAudioBufferInfo
{
    DISPID_SABIMinNotification = 1,
    DISPID_SABIBufferSize = 2,
    DISPID_SABIEventBias = 3
};

enum DISPID_SpeechWaveFormatEx
{
    DISPID_SWFEFormatTag = 1,
    DISPID_SWFEChannels = 2,
    DISPID_SWFESamplesPerSec = 3,
    DISPID_SWFEAvgBytesPerSec = 4,
    DISPID_SWFEBlockAlign = 5,
    DISPID_SWFEBitsPerSample = 6,
    DISPID_SWFEExtraData = 7
};

enum DISPID_SpeechVoice
{
    DISPID_SVStatus = 1,
    DISPID_SVVoice = 2,
    DISPID_SVAudioOutput = 3,
    DISPID_SVAudioOutputStream = 4,
    DISPID_SVRate = 5,
    DISPID_SVVolume = 6,
    DISPID_SVAllowAudioOuputFormatChangesOnNextSet = 7,
    DISPID_SVEventInterests = 8,
    DISPID_SVPriority = 9,
    DISPID_SVAlertBoundary = 10,
    DISPID_SVSyncronousSpeakTimeout = 11,
    DISPID_SVSpeak = 12,
    DISPID_SVSpeakStream = 13,
    DISPID_SVPause = 14,
    DISPID_SVResume = 15,
    DISPID_SVSkip = 16,
    DISPID_SVGetVoices = 17,
    DISPID_SVGetAudioOutputs = 18,
    DISPID_SVWaitUntilDone = 19,
    DISPID_SVSpeakCompleteEvent = 20,
    DISPID_SVIsUISupported = 21,
    DISPID_SVDisplayUI = 22
};

enum DISPID_SpeechVoiceStatus
{
    DISPID_SVSCurrentStreamNumber = 1,
    DISPID_SVSLastStreamNumberQueued = 2,
    DISPID_SVSLastResult = 3,
    DISPID_SVSRunningState = 4,
    DISPID_SVSInputWordPosition = 5,
    DISPID_SVSInputWordLength = 6,
    DISPID_SVSInputSentencePosition = 7,
    DISPID_SVSInputSentenceLength = 8,
    DISPID_SVSLastBookmark = 9,
    DISPID_SVSLastBookmarkId = 10,
    DISPID_SVSPhonemeId = 11,
    DISPID_SVSVisemeId = 12
};

enum DISPID_SpeechVoiceEvent
{
    DISPID_SVEStreamStart = 1,
    DISPID_SVEStreamEnd = 2,
    DISPID_SVEVoiceChange = 3,
    DISPID_SVEBookmark = 4,
    DISPID_SVEWord = 5,
    DISPID_SVEPhoneme = 6,
    DISPID_SVESentenceBoundary = 7,
    DISPID_SVEViseme = 8,
    DISPID_SVEAudioLevel = 9,
    DISPID_SVEEnginePrivate = 10
};

enum DISPID_SpeechRecognizer
{
    DISPID_SRRecognizer = 1,
    DISPID_SRAllowAudioInputFormatChangesOnNextSet = 2,
    DISPID_SRAudioInput = 3,
    DISPID_SRAudioInputStream = 4,
    DISPID_SRIsShared = 5,
    DISPID_SRState = 6,
    DISPID_SRStatus = 7,
    DISPID_SRProfile = 8,
    DISPID_SREmulateRecognition = 9,
    DISPID_SRCreateRecoContext = 10,
    DISPID_SRGetFormat = 11,
    DISPID_SRSetPropertyNumber = 12,
    DISPID_SRGetPropertyNumber = 13,
    DISPID_SRSetPropertyString = 14,
    DISPID_SRGetPropertyString = 15,
    DISPID_SRIsUISupported = 16,
    DISPID_SRDisplayUI = 17,
    DISPID_SRGetRecognizers = 18,
    DISPID_SVGetAudioInputs = 19,
    DISPID_SVGetProfiles = 20
};

enum SpeechEmulationCompareFlags
{
    SECFIgnoreCase = 1,
    SECFIgnoreKanaType = 65536,
    SECFIgnoreWidth = 131072,
    SECFNoSpecialChars = 536870912,
    SECFEmulateResult = 1073741824,
    SECFDefault = 196609
};

enum DISPID_SpeechRecognizerStatus
{
    DISPID_SRSAudioStatus = 1,
    DISPID_SRSCurrentStreamPosition = 2,
    DISPID_SRSCurrentStreamNumber = 3,
    DISPID_SRSNumberOfActiveRules = 4,
    DISPID_SRSClsidEngine = 5,
    DISPID_SRSSupportedLanguages = 6
};

enum DISPID_SpeechRecoContext
{
    DISPID_SRCRecognizer = 1,
    DISPID_SRCAudioInInterferenceStatus = 2,
    DISPID_SRCRequestedUIType = 3,
    DISPID_SRCVoice = 4,
    DISPID_SRAllowVoiceFormatMatchingOnNextSet = 5,
    DISPID_SRCVoicePurgeEvent = 6,
    DISPID_SRCEventInterests = 7,
    DISPID_SRCCmdMaxAlternates = 8,
    DISPID_SRCState = 9,
    DISPID_SRCRetainedAudio = 10,
    DISPID_SRCRetainedAudioFormat = 11,
    DISPID_SRCPause = 12,
    DISPID_SRCResume = 13,
    DISPID_SRCCreateGrammar = 14,
    DISPID_SRCCreateResultFromMemory = 15,
    DISPID_SRCBookmark = 16,
    DISPID_SRCSetAdaptationData = 17
};

enum DISPIDSPRG
{
    DISPID_SRGId = 1,
    DISPID_SRGRecoContext = 2,
    DISPID_SRGState = 3,
    DISPID_SRGRules = 4,
    DISPID_SRGReset = 5,
    DISPID_SRGCommit = 6,
    DISPID_SRGCmdLoadFromFile = 7,
    DISPID_SRGCmdLoadFromObject = 8,
    DISPID_SRGCmdLoadFromResource = 9,
    DISPID_SRGCmdLoadFromMemory = 10,
    DISPID_SRGCmdLoadFromProprietaryGrammar = 11,
    DISPID_SRGCmdSetRuleState = 12,
    DISPID_SRGCmdSetRuleIdState = 13,
    DISPID_SRGDictationLoad = 14,
    DISPID_SRGDictationUnload = 15,
    DISPID_SRGDictationSetState = 16,
    DISPID_SRGSetWordSequenceData = 17,
    DISPID_SRGSetTextSelection = 18,
    DISPID_SRGIsPronounceable = 19
};

enum DISPID_SpeechRecoContextEvents
{
    DISPID_SRCEStartStream = 1,
    DISPID_SRCEEndStream = 2,
    DISPID_SRCEBookmark = 3,
    DISPID_SRCESoundStart = 4,
    DISPID_SRCESoundEnd = 5,
    DISPID_SRCEPhraseStart = 6,
    DISPID_SRCERecognition = 7,
    DISPID_SRCEHypothesis = 8,
    DISPID_SRCEPropertyNumberChange = 9,
    DISPID_SRCEPropertyStringChange = 10,
    DISPID_SRCEFalseRecognition = 11,
    DISPID_SRCEInterference = 12,
    DISPID_SRCERequestUI = 13,
    DISPID_SRCERecognizerStateChange = 14,
    DISPID_SRCEAdaptation = 15,
    DISPID_SRCERecognitionForOtherContext = 16,
    DISPID_SRCEAudioLevel = 17,
    DISPID_SRCEEnginePrivate = 18
};

enum DISPID_SpeechGrammarRule
{
    DISPID_SGRAttributes = 1,
    DISPID_SGRInitialState = 2,
    DISPID_SGRName = 3,
    DISPID_SGRId = 4,
    DISPID_SGRClear = 5,
    DISPID_SGRAddResource = 6,
    DISPID_SGRAddState = 7
};

enum DISPID_SpeechGrammarRules
{
    DISPID_SGRsCount = 1,
    DISPID_SGRsDynamic = 2,
    DISPID_SGRsAdd = 3,
    DISPID_SGRsCommit = 4,
    DISPID_SGRsCommitAndSave = 5,
    DISPID_SGRsFindRule = 6,
    DISPID_SGRsItem = 0,
    DISPID_SGRs_NewEnum = -4
};

enum DISPID_SpeechGrammarRuleState
{
    DISPID_SGRSRule = 1,
    DISPID_SGRSTransitions = 2,
    DISPID_SGRSAddWordTransition = 3,
    DISPID_SGRSAddRuleTransition = 4,
    DISPID_SGRSAddSpecialTransition = 5
};

enum DISPID_SpeechGrammarRuleStateTransitions
{
    DISPID_SGRSTsCount = 1,
    DISPID_SGRSTsItem = 0,
    DISPID_SGRSTs_NewEnum = -4
};

enum DISPID_SpeechGrammarRuleStateTransition
{
    DISPID_SGRSTType = 1,
    DISPID_SGRSTText = 2,
    DISPID_SGRSTRule = 3,
    DISPID_SGRSTWeight = 4,
    DISPID_SGRSTPropertyName = 5,
    DISPID_SGRSTPropertyId = 6,
    DISPID_SGRSTPropertyValue = 7,
    DISPID_SGRSTNextState = 8
};

enum DISPIDSPTSI
{
    DISPIDSPTSI_ActiveOffset = 1,
    DISPIDSPTSI_ActiveLength = 2,
    DISPIDSPTSI_SelectionOffset = 3,
    DISPIDSPTSI_SelectionLength = 4
};

enum DISPID_SpeechRecoResult
{
    DISPID_SRRRecoContext = 1,
    DISPID_SRRTimes = 2,
    DISPID_SRRAudioFormat = 3,
    DISPID_SRRPhraseInfo = 4,
    DISPID_SRRAlternates = 5,
    DISPID_SRRAudio = 6,
    DISPID_SRRSpeakAudio = 7,
    DISPID_SRRSaveToMemory = 8,
    DISPID_SRRDiscardResultInfo = 9
};

enum DISPID_SpeechXMLRecoResult
{
    DISPID_SRRGetXMLResult = 10,
    DISPID_SRRGetXMLErrorInfo = 11
};

enum SPXMLRESULTOPTIONS
{
    SPXRO_SML = 0,
    SPXRO_Alternates_SML = 1
};

enum DISPID_SpeechRecoResult2
{
    DISPID_SRRSetTextFeedback = 12
};

enum DISPID_SpeechPhraseBuilder
{
    DISPID_SPPBRestorePhraseFromMemory = 1
};

enum DISPID_SpeechRecoResultTimes
{
    DISPID_SRRTStreamTime = 1,
    DISPID_SRRTLength = 2,
    DISPID_SRRTTickCount = 3,
    DISPID_SRRTOffsetFromStart = 4
};

enum DISPID_SpeechPhraseAlternate
{
    DISPID_SPARecoResult = 1,
    DISPID_SPAStartElementInResult = 2,
    DISPID_SPANumberOfElementsInResult = 3,
    DISPID_SPAPhraseInfo = 4,
    DISPID_SPACommit = 5
};

enum DISPID_SpeechPhraseAlternates
{
    DISPID_SPAsCount = 1,
    DISPID_SPAsItem = 0,
    DISPID_SPAs_NewEnum = -4
};

enum DISPID_SpeechPhraseInfo
{
    DISPID_SPILanguageId = 1,
    DISPID_SPIGrammarId = 2,
    DISPID_SPIStartTime = 3,
    DISPID_SPIAudioStreamPosition = 4,
    DISPID_SPIAudioSizeBytes = 5,
    DISPID_SPIRetainedSizeBytes = 6,
    DISPID_SPIAudioSizeTime = 7,
    DISPID_SPIRule = 8,
    DISPID_SPIProperties = 9,
    DISPID_SPIElements = 10,
    DISPID_SPIReplacements = 11,
    DISPID_SPIEngineId = 12,
    DISPID_SPIEnginePrivateData = 13,
    DISPID_SPISaveToMemory = 14,
    DISPID_SPIGetText = 15,
    DISPID_SPIGetDisplayAttributes = 16
};

enum DISPID_SpeechPhraseElement
{
    DISPID_SPEAudioTimeOffset = 1,
    DISPID_SPEAudioSizeTime = 2,
    DISPID_SPEAudioStreamOffset = 3,
    DISPID_SPEAudioSizeBytes = 4,
    DISPID_SPERetainedStreamOffset = 5,
    DISPID_SPERetainedSizeBytes = 6,
    DISPID_SPEDisplayText = 7,
    DISPID_SPELexicalForm = 8,
    DISPID_SPEPronunciation = 9,
    DISPID_SPEDisplayAttributes = 10,
    DISPID_SPERequiredConfidence = 11,
    DISPID_SPEActualConfidence = 12,
    DISPID_SPEEngineConfidence = 13
};

enum DISPID_SpeechPhraseElements
{
    DISPID_SPEsCount = 1,
    DISPID_SPEsItem = 0,
    DISPID_SPEs_NewEnum = -4
};

enum DISPID_SpeechPhraseReplacement
{
    DISPID_SPRDisplayAttributes = 1,
    DISPID_SPRText = 2,
    DISPID_SPRFirstElement = 3,
    DISPID_SPRNumberOfElements = 4
};

enum DISPID_SpeechPhraseReplacements
{
    DISPID_SPRsCount = 1,
    DISPID_SPRsItem = 0,
    DISPID_SPRs_NewEnum = -4
};

enum DISPID_SpeechPhraseProperty
{
    DISPID_SPPName = 1,
    DISPID_SPPId = 2,
    DISPID_SPPValue = 3,
    DISPID_SPPFirstElement = 4,
    DISPID_SPPNumberOfElements = 5,
    DISPID_SPPEngineConfidence = 6,
    DISPID_SPPConfidence = 7,
    DISPID_SPPParent = 8,
    DISPID_SPPChildren = 9
};

enum DISPID_SpeechPhraseProperties
{
    DISPID_SPPsCount = 1,
    DISPID_SPPsItem = 0,
    DISPID_SPPs_NewEnum = -4
};

enum DISPID_SpeechPhraseRule
{
    DISPID_SPRuleName = 1,
    DISPID_SPRuleId = 2,
    DISPID_SPRuleFirstElement = 3,
    DISPID_SPRuleNumberOfElements = 4,
    DISPID_SPRuleParent = 5,
    DISPID_SPRuleChildren = 6,
    DISPID_SPRuleConfidence = 7,
    DISPID_SPRuleEngineConfidence = 8
};

enum DISPID_SpeechPhraseRules
{
    DISPID_SPRulesCount = 1,
    DISPID_SPRulesItem = 0,
    DISPID_SPRules_NewEnum = -4
};

enum DISPID_SpeechLexicon
{
    DISPID_SLGenerationId = 1,
    DISPID_SLGetWords = 2,
    DISPID_SLAddPronunciation = 3,
    DISPID_SLAddPronunciationByPhoneIds = 4,
    DISPID_SLRemovePronunciation = 5,
    DISPID_SLRemovePronunciationByPhoneIds = 6,
    DISPID_SLGetPronunciations = 7,
    DISPID_SLGetGenerationChange = 8
};

enum DISPID_SpeechLexiconWords
{
    DISPID_SLWsCount = 1,
    DISPID_SLWsItem = 0,
    DISPID_SLWs_NewEnum = -4
};

enum DISPID_SpeechLexiconWord
{
    DISPID_SLWLangId = 1,
    DISPID_SLWType = 2,
    DISPID_SLWWord = 3,
    DISPID_SLWPronunciations = 4
};

enum DISPID_SpeechLexiconProns
{
    DISPID_SLPsCount = 1,
    DISPID_SLPsItem = 0,
    DISPID_SLPs_NewEnum = -4
};

enum DISPID_SpeechLexiconPronunciation
{
    DISPID_SLPType = 1,
    DISPID_SLPLangId = 2,
    DISPID_SLPPartOfSpeech = 3,
    DISPID_SLPPhoneIds = 4,
    DISPID_SLPSymbolic = 5
};

enum DISPID_SpeechPhoneConverter
{
    DISPID_SPCLangId = 1,
    DISPID_SPCPhoneToId = 2,
    DISPID_SPCIdToPhone = 3
};

struct __declspec(uuid("c3e4f353-433f-43d6-89a1-6a62a7054c3d"))
ISpeechPhoneConverter : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_LanguageId (
        /*[out,retval]*/ long * LanguageId ) = 0;
      virtual HRESULT __stdcall put_LanguageId (
        /*[in]*/ long LanguageId ) = 0;
      virtual HRESULT __stdcall PhoneToId (
        /*[in]*/ BSTR Phonemes,
        /*[out,retval]*/ VARIANT * IdArray ) = 0;
      virtual HRESULT __stdcall IdToPhone (
        /*[in]*/ VARIANT IdArray,
        /*[out,retval]*/ BSTR * Phonemes ) = 0;
};

struct __declspec(uuid("e2ae5372-5d40-11d2-960e-00c04f8ee628"))
SpNotifyTranslator;
    // [ default ] interface ISpNotifyTranslator

struct __declspec(uuid("259684dc-37c3-11d2-9603-00c04f8ee628"))
ISpNotifySink : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Notify ( ) = 0;
};

struct __declspec(uuid("aca16614-5d3d-11d2-960e-00c04f8ee628"))
ISpNotifyTranslator : ISpNotifySink
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall InitWindowMessage (
        /*[in]*/ wireHWND hWnd,
        /*[in]*/ unsigned int Msg,
        /*[in]*/ UINT_PTR wParam,
        /*[in]*/ LONG_PTR lParam ) = 0;
      virtual HRESULT __stdcall InitCallback (
        /*[in]*/ void * * pfnCallback,
        /*[in]*/ UINT_PTR wParam,
        /*[in]*/ LONG_PTR lParam ) = 0;
      virtual HRESULT __stdcall InitSpNotifyCallback (
        /*[in]*/ void * * pSpCallback,
        /*[in]*/ UINT_PTR wParam,
        /*[in]*/ LONG_PTR lParam ) = 0;
      virtual HRESULT __stdcall InitWin32Event (
        void * hEvent,
        long fCloseHandleOnRelease ) = 0;
      virtual HRESULT __stdcall Wait (
        /*[in]*/ unsigned long dwMilliseconds ) = 0;
      virtual void * __stdcall GetEventHandle ( ) = 0;
};

struct __declspec(uuid("a910187f-0c7a-45ac-92cc-59edafb77b53"))
SpObjectTokenCategory;
    // [ default ] interface ISpeechObjectTokenCategory
    // interface ISpObjectTokenCategory

struct __declspec(uuid("14056581-e16c-11d2-bb90-00c04f8ee6c0"))
ISpDataKey : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetData (
        LPWSTR pszValueName,
        unsigned long cbData,
        unsigned char * pData ) = 0;
      virtual HRESULT __stdcall GetData (
        LPWSTR pszValueName,
        unsigned long * pcbData,
        unsigned char * pData ) = 0;
      virtual HRESULT __stdcall SetStringValue (
        /*[in]*/ LPWSTR pszValueName,
        LPWSTR pszValue ) = 0;
      virtual HRESULT __stdcall GetStringValue (
        /*[in]*/ LPWSTR pszValueName,
        /*[out]*/ LPWSTR * ppszValue ) = 0;
      virtual HRESULT __stdcall SetDWORD (
        LPWSTR pszValueName,
        unsigned long dwValue ) = 0;
      virtual HRESULT __stdcall GetDWORD (
        LPWSTR pszValueName,
        unsigned long * pdwValue ) = 0;
      virtual HRESULT __stdcall OpenKey (
        LPWSTR pszSubKeyName,
        struct ISpDataKey * * ppSubKey ) = 0;
      virtual HRESULT __stdcall CreateKey (
        LPWSTR pszSubKey,
        struct ISpDataKey * * ppSubKey ) = 0;
      virtual HRESULT __stdcall DeleteKey (
        LPWSTR pszSubKey ) = 0;
      virtual HRESULT __stdcall DeleteValue (
        /*[in]*/ LPWSTR pszValueName ) = 0;
      virtual HRESULT __stdcall EnumKeys (
        unsigned long Index,
        /*[out]*/ LPWSTR * ppszSubKeyName ) = 0;
      virtual HRESULT __stdcall EnumValues (
        unsigned long Index,
        /*[out]*/ LPWSTR * ppszValueName ) = 0;
};

enum SPDATAKEYLOCATION
{
    SPDKL_DefaultLocation = 0,
    SPDKL_CurrentUser = 1,
    SPDKL_LocalMachine = 2,
    SPDKL_CurrentConfig = 5
};

struct __declspec(uuid("ef411752-3736-4cb4-9c8c-8ef4ccb58efe"))
SpObjectToken;
    // [ default ] interface ISpeechObjectToken
    // interface ISpObjectToken

struct __declspec(uuid("96749373-3391-11d2-9ee3-00c04f797396"))
SpResourceManager;
    // [ default ] interface ISpResourceManager

struct __declspec(uuid("93384e18-5014-43d5-adbb-a78e055926bd"))
ISpResourceManager : IServiceProvider
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetObject (
        /*[in]*/ GUID * guidServiceId,
        /*[in]*/ IUnknown * punkObject ) = 0;
      virtual HRESULT __stdcall GetObject (
        /*[in]*/ GUID * guidServiceId,
        /*[in]*/ GUID * ObjectCLSID,
        /*[in]*/ GUID * ObjectIID,
        /*[in]*/ long fReleaseWhenLastExternalRefReleased,
        /*[out]*/ void * * ppObject ) = 0;
};

struct __declspec(uuid("7013943a-e2ec-11d2-a086-00c04f8ef9b5"))
SpStreamFormatConverter;
    // [ default ] interface ISpStreamFormatConverter

#pragma pack(push, 4)

struct WaveFormatEx
{
    unsigned short wFormatTag;
    unsigned short nChannels;
    unsigned long nSamplesPerSec;
    unsigned long nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short wBitsPerSample;
    unsigned short cbSize;
};

#pragma pack(pop)

struct __declspec(uuid("bed530be-2606-4f4d-a1c0-54c5cda5566f"))
ISpStreamFormat : IStream
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetFormat (
        GUID * pguidFormatId,
        struct WaveFormatEx * * ppCoMemWaveFormatEx ) = 0;
};

struct __declspec(uuid("678a932c-ea71-4446-9b41-78fda6280a29"))
ISpStreamFormatConverter : ISpStreamFormat
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetBaseStream (
        /*[in]*/ struct ISpStreamFormat * pStream,
        /*[in]*/ long fSetFormatToBaseStreamFormat,
        /*[in]*/ long fWriteToBaseStream ) = 0;
      virtual HRESULT __stdcall GetBaseStream (
        /*[out]*/ struct ISpStreamFormat * * ppStream ) = 0;
      virtual HRESULT __stdcall SetFormat (
        /*[in]*/ GUID * rguidFormatIdOfConvertedStream,
        /*[in]*/ struct WaveFormatEx * pWaveFormatExOfConvertedStream ) = 0;
      virtual HRESULT __stdcall ResetSeekPosition ( ) = 0;
      virtual HRESULT __stdcall ScaleConvertedToBaseOffset (
        /*[in]*/ unsigned __int64 ullOffsetConvertedStream,
        /*[out]*/ unsigned __int64 * pullOffsetBaseStream ) = 0;
      virtual HRESULT __stdcall ScaleBaseToConvertedOffset (
        /*[in]*/ unsigned __int64 ullOffsetBaseStream,
        /*[out]*/ unsigned __int64 * pullOffsetConvertedStream ) = 0;
};

struct __declspec(uuid("ab1890a0-e91f-11d2-bb91-00c04f8ee6c0"))
SpMMAudioEnum;
    // [ default ] interface IEnumSpObjectTokens

struct __declspec(uuid("cf3d2e50-53f2-11d2-960c-00c04f8ee628"))
SpMMAudioIn;
    // [ default ] interface ISpeechMMSysAudio
    // interface ISpEventSource
    // interface ISpEventSink
    // interface ISpObjectWithToken
    // interface ISpMMSysAudio

struct __declspec(uuid("5eff4aef-8487-11d2-961c-00c04f8ee628"))
ISpNotifySource : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetNotifySink (
        /*[in]*/ struct ISpNotifySink * pNotifySink ) = 0;
      virtual HRESULT __stdcall SetNotifyWindowMessage (
        /*[in]*/ wireHWND hWnd,
        /*[in]*/ unsigned int Msg,
        /*[in]*/ UINT_PTR wParam,
        /*[in]*/ LONG_PTR lParam ) = 0;
      virtual HRESULT __stdcall SetNotifyCallbackFunction (
        /*[in]*/ void * * pfnCallback,
        /*[in]*/ UINT_PTR wParam,
        /*[in]*/ LONG_PTR lParam ) = 0;
      virtual HRESULT __stdcall SetNotifyCallbackInterface (
        /*[in]*/ void * * pSpCallback,
        /*[in]*/ UINT_PTR wParam,
        /*[in]*/ LONG_PTR lParam ) = 0;
      virtual HRESULT __stdcall SetNotifyWin32Event ( ) = 0;
      virtual HRESULT __stdcall WaitForNotifyEvent (
        /*[in]*/ unsigned long dwMilliseconds ) = 0;
      virtual void * __stdcall GetNotifyEventHandle ( ) = 0;
};

#pragma pack(push, 8)

struct SPEVENT
{
    unsigned short eEventId;
    unsigned short elParamType;
    unsigned long ulStreamNum;
    unsigned __int64 ullAudioStreamOffset;
    UINT_PTR wParam;
    LONG_PTR lParam;
};

#pragma pack(pop)

#pragma pack(push, 8)

struct SPEVENTSOURCEINFO
{
    unsigned __int64 ullEventInterest;
    unsigned __int64 ullQueuedInterest;
    unsigned long ulCount;
};

#pragma pack(pop)

struct __declspec(uuid("be7a9cce-5f9e-11d2-960f-00c04f8ee628"))
ISpEventSource : ISpNotifySource
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetInterest (
        /*[in]*/ unsigned __int64 ullEventInterest,
        /*[in]*/ unsigned __int64 ullQueuedInterest ) = 0;
      virtual HRESULT __stdcall GetEvents (
        /*[in]*/ unsigned long ulCount,
        /*[out]*/ struct SPEVENT * pEventArray,
        /*[out]*/ unsigned long * pulFetched ) = 0;
      virtual HRESULT __stdcall GetInfo (
        /*[out]*/ struct SPEVENTSOURCEINFO * pInfo ) = 0;
};

struct __declspec(uuid("be7a9cc9-5f9e-11d2-960f-00c04f8ee628"))
ISpEventSink : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall AddEvents (
        /*[in]*/ struct SPEVENT * pEventArray,
        /*[in]*/ unsigned long ulCount ) = 0;
      virtual HRESULT __stdcall GetEventInterest (
        /*[out]*/ unsigned __int64 * pullEventInterest ) = 0;
};

enum _SPAUDIOSTATE
{
    SPAS_CLOSED = 0,
    SPAS_STOP = 1,
    SPAS_PAUSE = 2,
    SPAS_RUN = 3
};

#pragma pack(push, 8)

struct SPAUDIOSTATUS
{
    long cbFreeBuffSpace;
    unsigned long cbNonBlockingIO;
    SPAUDIOSTATE State;
    unsigned __int64 CurSeekPos;
    unsigned __int64 CurDevicePos;
    unsigned long dwAudioLevel;
    unsigned long dwReserved2;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPAUDIOBUFFERINFO
{
    unsigned long ulMsMinNotification;
    unsigned long ulMsBufferSize;
    unsigned long ulMsEventBias;
};

#pragma pack(pop)

struct __declspec(uuid("c05c768f-fae8-4ec2-8e07-338321c12452"))
ISpAudio : ISpStreamFormat
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetState (
        /*[in]*/ SPAUDIOSTATE NewState,
        /*[in]*/ unsigned __int64 ullReserved ) = 0;
      virtual HRESULT __stdcall SetFormat (
        /*[in]*/ GUID * rguidFmtId,
        /*[in]*/ struct WaveFormatEx * pWaveFormatEx ) = 0;
      virtual HRESULT __stdcall GetStatus (
        /*[out]*/ struct SPAUDIOSTATUS * pStatus ) = 0;
      virtual HRESULT __stdcall SetBufferInfo (
        /*[in]*/ struct SPAUDIOBUFFERINFO * pBuffInfo ) = 0;
      virtual HRESULT __stdcall GetBufferInfo (
        /*[out]*/ struct SPAUDIOBUFFERINFO * pBuffInfo ) = 0;
      virtual HRESULT __stdcall GetDefaultFormat (
        /*[out]*/ GUID * pFormatId,
        /*[out]*/ struct WaveFormatEx * * ppCoMemWaveFormatEx ) = 0;
      virtual void * __stdcall EventHandle ( ) = 0;
      virtual HRESULT __stdcall GetVolumeLevel (
        /*[out]*/ unsigned long * pLevel ) = 0;
      virtual HRESULT __stdcall SetVolumeLevel (
        /*[in]*/ unsigned long Level ) = 0;
      virtual HRESULT __stdcall GetBufferNotifySize (
        /*[out]*/ unsigned long * pcbSize ) = 0;
      virtual HRESULT __stdcall SetBufferNotifySize (
        /*[in]*/ unsigned long cbSize ) = 0;
};

struct __declspec(uuid("15806f6e-1d70-4b48-98e6-3b1a007509ab"))
ISpMMSysAudio : ISpAudio
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetDeviceId (
        /*[out]*/ unsigned int * puDeviceId ) = 0;
      virtual HRESULT __stdcall SetDeviceId (
        /*[in]*/ unsigned int uDeviceId ) = 0;
      virtual HRESULT __stdcall GetMMHandle (
        void * * pHandle ) = 0;
      virtual HRESULT __stdcall GetLineId (
        /*[out]*/ unsigned int * puLineId ) = 0;
      virtual HRESULT __stdcall SetLineId (
        /*[in]*/ unsigned int uLineId ) = 0;
};

struct __declspec(uuid("a8c680eb-3d32-11d2-9ee7-00c04f797396"))
SpMMAudioOut;
    // [ default ] interface ISpeechMMSysAudio
    // interface ISpEventSource
    // interface ISpEventSink
    // interface ISpObjectWithToken
    // interface ISpMMSysAudio

struct __declspec(uuid("715d9c59-4442-11d2-9605-00c04f8ee628"))
SpStream;
    // [ default ] interface ISpStream

enum SPFILEMODE
{
    SPFM_OPEN_READONLY = 0,
    SPFM_OPEN_READWRITE = 1,
    SPFM_CREATE = 2,
    SPFM_CREATE_ALWAYS = 3,
    SPFM_NUM_MODES = 4
};

struct __declspec(uuid("12e3cca9-7518-44c5-a5e7-ba5a79cb929e"))
ISpStream : ISpStreamFormat
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetBaseStream (
        struct IStream * pStream,
        GUID * rguidFormat,
        struct WaveFormatEx * pWaveFormatEx ) = 0;
      virtual HRESULT __stdcall GetBaseStream (
        struct IStream * * ppStream ) = 0;
      virtual HRESULT __stdcall BindToFile (
        LPWSTR pszFileName,
        enum SPFILEMODE eMode,
        GUID * pFormatId,
        struct WaveFormatEx * pWaveFormatEx,
        unsigned __int64 ullEventInterest ) = 0;
      virtual HRESULT __stdcall Close ( ) = 0;
};

struct __declspec(uuid("96749377-3391-11d2-9ee3-00c04f797396"))
SpVoice;
    // [ default ] interface ISpeechVoice
    // interface ISpVoice
    // interface ISpPhoneticAlphabetSelection
    // [ default, source ] dispinterface _ISpeechVoiceEvents

enum SPVISEMES
{
    SP_VISEME_0 = 0,
    SP_VISEME_1 = 1,
    SP_VISEME_2 = 2,
    SP_VISEME_3 = 3,
    SP_VISEME_4 = 4,
    SP_VISEME_5 = 5,
    SP_VISEME_6 = 6,
    SP_VISEME_7 = 7,
    SP_VISEME_8 = 8,
    SP_VISEME_9 = 9,
    SP_VISEME_10 = 10,
    SP_VISEME_11 = 11,
    SP_VISEME_12 = 12,
    SP_VISEME_13 = 13,
    SP_VISEME_14 = 14,
    SP_VISEME_15 = 15,
    SP_VISEME_16 = 16,
    SP_VISEME_17 = 17,
    SP_VISEME_18 = 18,
    SP_VISEME_19 = 19,
    SP_VISEME_20 = 20,
    SP_VISEME_21 = 21
};

#pragma pack(push, 4)

struct SPVOICESTATUS
{
    unsigned long ulCurrentStream;
    unsigned long ulLastStreamQueued;
    HRESULT hrLastResult;
    unsigned long dwRunningState;
    unsigned long ulInputWordPos;
    unsigned long ulInputWordLen;
    unsigned long ulInputSentPos;
    unsigned long ulInputSentLen;
    long lBookmarkId;
    unsigned short PhonemeId;
    enum SPVISEMES VisemeId;
    unsigned long dwReserved1;
    unsigned long dwReserved2;
};

#pragma pack(pop)

enum SPVPRIORITY
{
    SPVPRI_NORMAL = 0,
    SPVPRI_ALERT = 1,
    SPVPRI_OVER = 2
};

enum SPEVENTENUM
{
    SPEI_UNDEFINED = 0,
    SPEI_START_INPUT_STREAM = 1,
    SPEI_END_INPUT_STREAM = 2,
    SPEI_VOICE_CHANGE = 3,
    SPEI_TTS_BOOKMARK = 4,
    SPEI_WORD_BOUNDARY = 5,
    SPEI_PHONEME = 6,
    SPEI_SENTENCE_BOUNDARY = 7,
    SPEI_VISEME = 8,
    SPEI_TTS_AUDIO_LEVEL = 9,
    SPEI_TTS_PRIVATE = 15,
    SPEI_MIN_TTS = 1,
    SPEI_MAX_TTS = 15,
    SPEI_END_SR_STREAM = 34,
    SPEI_SOUND_START = 35,
    SPEI_SOUND_END = 36,
    SPEI_PHRASE_START = 37,
    SPEI_RECOGNITION = 38,
    SPEI_HYPOTHESIS = 39,
    SPEI_SR_BOOKMARK = 40,
    SPEI_PROPERTY_NUM_CHANGE = 41,
    SPEI_PROPERTY_STRING_CHANGE = 42,
    SPEI_FALSE_RECOGNITION = 43,
    SPEI_INTERFERENCE = 44,
    SPEI_REQUEST_UI = 45,
    SPEI_RECO_STATE_CHANGE = 46,
    SPEI_ADAPTATION = 47,
    SPEI_START_SR_STREAM = 48,
    SPEI_RECO_OTHER_CONTEXT = 49,
    SPEI_SR_AUDIO_LEVEL = 50,
    SPEI_SR_RETAINEDAUDIO = 51,
    SPEI_SR_PRIVATE = 52,
    SPEI_RESERVED4 = 53,
    SPEI_RESERVED5 = 54,
    SPEI_RESERVED6 = 55,
    SPEI_MIN_SR = 34,
    SPEI_MAX_SR = 55,
    SPEI_RESERVED1 = 30,
    SPEI_RESERVED2 = 33,
    SPEI_RESERVED3 = 63
};

struct __declspec(uuid("b2745efd-42ce-48ca-81f1-a96e02538a90"))
ISpPhoneticAlphabetSelection : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall IsAlphabetUPS (
        /*[out]*/ long * pfIsUPS ) = 0;
      virtual HRESULT __stdcall SetAlphabetToUPS (
        long fForceUPS ) = 0;
};

struct __declspec(uuid("47206204-5eca-11d2-960f-00c04f8ee628"))
SpSharedRecoContext;
    // [ default ] interface ISpeechRecoContext
    // interface ISpRecoContext
    // interface ISpRecoContext2
    // interface ISpPhoneticAlphabetSelection
    // [ default, source ] dispinterface _ISpeechRecoContextEvents

struct __declspec(uuid("5b4fb971-b115-4de1-ad97-e482e3bf6ee4"))
ISpProperties : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetPropertyNum (
        /*[in]*/ LPWSTR pName,
        /*[in]*/ long lValue ) = 0;
      virtual HRESULT __stdcall GetPropertyNum (
        /*[in]*/ LPWSTR pName,
        /*[out]*/ long * plValue ) = 0;
      virtual HRESULT __stdcall SetPropertyString (
        /*[in]*/ LPWSTR pName,
        /*[in]*/ LPWSTR pValue ) = 0;
      virtual HRESULT __stdcall GetPropertyString (
        /*[in]*/ LPWSTR pName,
        /*[out]*/ LPWSTR * ppCoMemValue ) = 0;
};

enum SPRECOSTATE
{
    SPRST_INACTIVE = 0,
    SPRST_ACTIVE = 1,
    SPRST_ACTIVE_ALWAYS = 2,
    SPRST_INACTIVE_WITH_PURGE = 3,
    SPRST_NUM_STATES = 4
};

#pragma pack(push, 8)

struct SPRECOGNIZERSTATUS
{
    struct SPAUDIOSTATUS AudioStatus;
    unsigned __int64 ullRecognitionStreamPos;
    unsigned long ulStreamNumber;
    unsigned long ulNumActive;
    GUID ClsidEngine;
    unsigned long cLangIDs;
    unsigned short aLangID[20];
    unsigned __int64 ullRecognitionStreamTime;
};

#pragma pack(pop)

enum SPWAVEFORMATTYPE
{
    SPWF_INPUT = 0,
    SPWF_SRENGINE = 1
};

#pragma pack(push, 4)

struct SPPHRASERULE
{
    LPWSTR pszName;
    unsigned long ulId;
    unsigned long ulFirstElement;
    unsigned long ulCountOfElements;
    struct SPPHRASERULE * pNextSibling;
    struct SPPHRASERULE * pFirstChild;
    float SREngineConfidence;
    char Confidence;
};

#pragma pack(pop)

#pragma pack(push, 8)

struct SPPHRASEPROPERTY
{
    LPWSTR pszName;
    unsigned long ulId;
    LPWSTR pszValue;
    VARIANT vValue;
    unsigned long ulFirstElement;
    unsigned long ulCountOfElements;
    struct SPPHRASEPROPERTY * pNextSibling;
    struct SPPHRASEPROPERTY * pFirstChild;
    float SREngineConfidence;
    char Confidence;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPPHRASEELEMENT
{
    unsigned long ulAudioTimeOffset;
    unsigned long ulAudioSizeTime;
    unsigned long ulAudioStreamOffset;
    unsigned long ulAudioSizeBytes;
    unsigned long ulRetainedStreamOffset;
    unsigned long ulRetainedSizeBytes;
    LPWSTR pszDisplayText;
    LPWSTR pszLexicalForm;
    unsigned short * pszPronunciation;
    unsigned char bDisplayAttributes;
    char RequiredConfidence;
    char ActualConfidence;
    unsigned char reserved;
    float SREngineConfidence;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPPHRASEREPLACEMENT
{
    unsigned char bDisplayAttributes;
    LPWSTR pszReplacementText;
    unsigned long ulFirstElement;
    unsigned long ulCountOfElements;
};

#pragma pack(pop)

#pragma pack(push, 8)

struct SPPHRASE
{
    unsigned long cbSize;
    unsigned short LangId;
    unsigned short wReserved;
    unsigned __int64 ullGrammarID;
    unsigned __int64 ftStartTime;
    unsigned __int64 ullAudioStreamPosition;
    unsigned long ulAudioSizeBytes;
    unsigned long ulRetainedSizeBytes;
    unsigned long ulAudioSizeTime;
    struct SPPHRASERULE Rule;
    struct SPPHRASEPROPERTY * pProperties;
    struct SPPHRASEELEMENT * pElements;
    unsigned long cReplacements;
    struct SPPHRASEREPLACEMENT * pReplacements;
    GUID SREngineID;
    unsigned long ulSREnginePrivateDataSize;
    unsigned char * pSREnginePrivateData;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPSERIALIZEDPHRASE
{
    unsigned long ulSerializedSize;
};

#pragma pack(pop)

struct __declspec(uuid("1a5c0354-b621-4b5a-8791-d306ed379e53"))
ISpPhrase : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetPhrase (
        /*[out]*/ struct SPPHRASE * * ppCoMemPhrase ) = 0;
      virtual HRESULT __stdcall GetSerializedPhrase (
        /*[out]*/ struct SPSERIALIZEDPHRASE * * ppCoMemPhrase ) = 0;
      virtual HRESULT __stdcall GetText (
        /*[in]*/ unsigned long ulStart,
        /*[in]*/ unsigned long ulCount,
        /*[in]*/ long fUseTextReplacements,
        /*[out]*/ LPWSTR * ppszCoMemText,
        /*[out]*/ unsigned char * pbDisplayAttributes ) = 0;
      virtual HRESULT __stdcall Discard (
        /*[in]*/ unsigned long dwValueTypes ) = 0;
};

enum SPGRAMMARWORDTYPE
{
    SPWT_DISPLAY = 0,
    SPWT_LEXICAL = 1,
    SPWT_PRONUNCIATION = 2,
    SPWT_LEXICAL_NO_SPECIAL_CHARS = 3
};

struct __declspec(uuid("8137828f-591a-4a42-be58-49ea7ebaac68"))
ISpGrammarBuilder : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall ResetGrammar (
        /*[in]*/ unsigned short NewLanguage ) = 0;
      virtual HRESULT __stdcall GetRule (
        /*[in]*/ LPWSTR pszRuleName,
        /*[in]*/ unsigned long dwRuleId,
        /*[in]*/ unsigned long dwAttributes,
        /*[in]*/ long fCreateIfNotExist,
        /*[out]*/ void * * phInitialState ) = 0;
      virtual HRESULT __stdcall ClearRule (
        void * hState ) = 0;
      virtual HRESULT __stdcall CreateNewState (
        void * hState,
        void * * phState ) = 0;
      virtual HRESULT __stdcall AddWordTransition (
        void * hFromState,
        void * hToState,
        LPWSTR psz,
        LPWSTR pszSeparators,
        enum SPGRAMMARWORDTYPE eWordType,
        float Weight,
        SPPROPERTYINFO * pPropInfo ) = 0;
      virtual HRESULT __stdcall AddRuleTransition (
        void * hFromState,
        void * hToState,
        void * hRule,
        float Weight,
        SPPROPERTYINFO * pPropInfo ) = 0;
      virtual HRESULT __stdcall AddResource (
        /*[in]*/ void * hRuleState,
        /*[in]*/ LPWSTR pszResourceName,
        /*[in]*/ LPWSTR pszResourceValue ) = 0;
      virtual HRESULT __stdcall Commit (
        unsigned long dwReserved ) = 0;
};

#pragma pack(push, 8)

struct tagSPPROPERTYINFO
{
    LPWSTR pszName;
    unsigned long ulId;
    LPWSTR pszValue;
    VARIANT vValue;
};

#pragma pack(pop)

enum SPLOADOPTIONS
{
    SPLO_STATIC = 0,
    SPLO_DYNAMIC = 1
};

#pragma pack(push, 4)

struct SPBINARYGRAMMAR
{
    unsigned long ulTotalSerializedSize;
};

#pragma pack(pop)

enum SPRULESTATE
{
    SPRS_INACTIVE = 0,
    SPRS_ACTIVE = 1,
    SPRS_ACTIVE_WITH_AUTO_PAUSE = 3,
    SPRS_ACTIVE_USER_DELIMITED = 4
};

#pragma pack(push, 4)

struct tagSPTEXTSELECTIONINFO
{
    unsigned long ulStartActiveOffset;
    unsigned long cchActiveChars;
    unsigned long ulStartSelection;
    unsigned long cchSelection;
};

#pragma pack(pop)

enum SPWORDPRONOUNCEABLE
{
    SPWP_UNKNOWN_WORD_UNPRONOUNCEABLE = 0,
    SPWP_UNKNOWN_WORD_PRONOUNCEABLE = 1,
    SPWP_KNOWN_WORD_PRONOUNCEABLE = 2
};

enum SPGRAMMARSTATE
{
    SPGS_DISABLED = 0,
    SPGS_ENABLED = 1,
    SPGS_EXCLUSIVE = 3
};

enum SPINTERFERENCE
{
    SPINTERFERENCE_NONE = 0,
    SPINTERFERENCE_NOISE = 1,
    SPINTERFERENCE_NOSIGNAL = 2,
    SPINTERFERENCE_TOOLOUD = 3,
    SPINTERFERENCE_TOOQUIET = 4,
    SPINTERFERENCE_TOOFAST = 5,
    SPINTERFERENCE_TOOSLOW = 6
};

#pragma pack(push, 4)

struct SPRECOCONTEXTSTATUS
{
    enum SPINTERFERENCE eInterference;
    unsigned short szRequestTypeOfUI[255];
    unsigned long dwReserved1;
    unsigned long dwReserved2;
};

#pragma pack(pop)

enum SPAUDIOOPTIONS
{
    SPAO_NONE = 0,
    SPAO_RETAIN_AUDIO = 1
};

#pragma pack(push, 4)

struct SPSERIALIZEDRESULT
{
    unsigned long ulSerializedSize;
};

#pragma pack(pop)

#pragma pack(push, 8)

struct SPRECORESULTTIMES
{
    struct _FILETIME ftStreamTime;
    unsigned __int64 ullLength;
    unsigned long dwTickCount;
    unsigned __int64 ullStart;
};

#pragma pack(pop)

struct __declspec(uuid("8fcebc98-4e49-4067-9c6c-d86a0e092e3d"))
ISpPhraseAlt : ISpPhrase
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetAltInfo (
        struct ISpPhrase * * ppParent,
        unsigned long * pulStartElementInParent,
        unsigned long * pcElementsInParent,
        unsigned long * pcElementsInAlt ) = 0;
      virtual HRESULT __stdcall Commit ( ) = 0;
};

enum SPBOOKMARKOPTIONS
{
    SPBO_NONE = 0,
    SPBO_PAUSE = 1,
    SPBO_AHEAD = 2,
    SPBO_TIME_UNITS = 4
};

enum SPCONTEXTSTATE
{
    SPCS_DISABLED = 0,
    SPCS_ENABLED = 1
};

enum SPADAPTATIONRELEVANCE
{
    SPAR_Unknown = 0,
    SPAR_Low = 1,
    SPAR_Medium = 2,
    SPAR_High = 3
};

struct __declspec(uuid("bead311c-52ff-437f-9464-6b21054ca73d"))
ISpRecoContext2 : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetGrammarOptions (
        /*[in]*/ unsigned long eGrammarOptions ) = 0;
      virtual HRESULT __stdcall GetGrammarOptions (
        /*[out]*/ unsigned long * peGrammarOptions ) = 0;
      virtual HRESULT __stdcall SetAdaptationData2 (
        /*[in]*/ LPWSTR pAdaptationData,
        unsigned long cch,
        /*[in]*/ LPWSTR pTopicName,
        unsigned long eAdaptationSettings,
        enum SPADAPTATIONRELEVANCE eRelevance ) = 0;
};

struct __declspec(uuid("41b89b6b-9399-11d2-9623-00c04f8ee628"))
SpInprocRecognizer;
    // [ default ] interface ISpeechRecognizer
    // interface ISpRecognizer
    // interface ISpRecognizer2
    // interface ISpSerializeState

struct __declspec(uuid("8fc6d974-c81e-4098-93c5-0147f61ed4d3"))
ISpRecognizer2 : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall EmulateRecognitionEx (
        /*[in]*/ struct ISpPhrase * pPhrase,
        /*[in]*/ unsigned long dwCompareFlags ) = 0;
      virtual HRESULT __stdcall SetTrainingState (
        long fDoingTraining,
        long fAdaptFromTrainingData ) = 0;
      virtual HRESULT __stdcall ResetAcousticModelAdaptation ( ) = 0;
};

struct __declspec(uuid("21b501a0-0ec7-46c9-92c3-a2bc784c54b9"))
ISpSerializeState : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetSerializedState (
        /*[out]*/ unsigned char * * ppbData,
        /*[out]*/ unsigned long * pulSize,
        /*[in]*/ unsigned long dwReserved ) = 0;
      virtual HRESULT __stdcall SetSerializedState (
        /*[in]*/ unsigned char * pbData,
        /*[in]*/ unsigned long ulSize,
        /*[in]*/ unsigned long dwReserved ) = 0;
};

struct __declspec(uuid("3bee4890-4fe9-4a37-8c1e-5e7e12791c1f"))
SpSharedRecognizer;
    // [ default ] interface ISpeechRecognizer
    // interface ISpRecognizer
    // interface ISpRecognizer2
    // interface ISpSerializeState

struct __declspec(uuid("0655e396-25d0-11d3-9c26-00c04f8ef87c"))
SpLexicon;
    // [ default ] interface ISpeechLexicon
    // interface ISpLexicon
    // interface ISpPhoneticAlphabetSelection

enum SPLEXICONTYPE
{
    eLEXTYPE_USER = 1,
    eLEXTYPE_APP = 2,
    eLEXTYPE_VENDORLEXICON = 4,
    eLEXTYPE_LETTERTOSOUND = 8,
    eLEXTYPE_MORPHOLOGY = 16,
    eLEXTYPE_RESERVED4 = 32,
    eLEXTYPE_USER_SHORTCUT = 64,
    eLEXTYPE_RESERVED6 = 128,
    eLEXTYPE_RESERVED7 = 256,
    eLEXTYPE_RESERVED8 = 512,
    eLEXTYPE_RESERVED9 = 1024,
    eLEXTYPE_RESERVED10 = 2048,
    eLEXTYPE_PRIVATE1 = 4096,
    eLEXTYPE_PRIVATE2 = 8192,
    eLEXTYPE_PRIVATE3 = 16384,
    eLEXTYPE_PRIVATE4 = 32768,
    eLEXTYPE_PRIVATE5 = 65536,
    eLEXTYPE_PRIVATE6 = 131072,
    eLEXTYPE_PRIVATE7 = 262144,
    eLEXTYPE_PRIVATE8 = 524288,
    eLEXTYPE_PRIVATE9 = 1048576,
    eLEXTYPE_PRIVATE10 = 2097152,
    eLEXTYPE_PRIVATE11 = 4194304,
    eLEXTYPE_PRIVATE12 = 8388608,
    eLEXTYPE_PRIVATE13 = 16777216,
    eLEXTYPE_PRIVATE14 = 33554432,
    eLEXTYPE_PRIVATE15 = 67108864,
    eLEXTYPE_PRIVATE16 = 134217728,
    eLEXTYPE_PRIVATE17 = 268435456,
    eLEXTYPE_PRIVATE18 = 536870912,
    eLEXTYPE_PRIVATE19 = 1073741824,
    eLEXTYPE_PRIVATE20 = 0x80000000
};

enum SPPARTOFSPEECH
{
    SPPS_NotOverriden = -1,
    SPPS_Unknown = 0,
    SPPS_Noun = 4096,
    SPPS_Verb = 8192,
    SPPS_Modifier = 12288,
    SPPS_Function = 16384,
    SPPS_Interjection = 20480,
    SPPS_Noncontent = 24576,
    SPPS_LMA = 28672,
    SPPS_SuppressWord = 61440
};

#pragma pack(push, 4)

struct SPWORDPRONUNCIATION
{
    struct SPWORDPRONUNCIATION * pNextWordPronunciation;
    enum SPLEXICONTYPE eLexiconType;
    unsigned short LangId;
    unsigned short wPronunciationFlags;
    enum SPPARTOFSPEECH ePartOfSpeech;
    unsigned short szPronunciation[1];
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPWORDPRONUNCIATIONLIST
{
    unsigned long ulSize;
    unsigned char * pvBuffer;
    struct SPWORDPRONUNCIATION * pFirstWordPronunciation;
};

#pragma pack(pop)

enum SPWORDTYPE
{
    eWORDTYPE_ADDED = 1,
    eWORDTYPE_DELETED = 2
};

#pragma pack(push, 4)

struct SPWORD
{
    struct SPWORD * pNextWord;
    unsigned short LangId;
    unsigned short wReserved;
    enum SPWORDTYPE eWordType;
    LPWSTR pszWord;
    struct SPWORDPRONUNCIATION * pFirstWordPronunciation;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPWORDLIST
{
    unsigned long ulSize;
    unsigned char * pvBuffer;
    struct SPWORD * pFirstWord;
};

#pragma pack(pop)

struct __declspec(uuid("da41a7c2-5383-4db2-916b-6c1719e3db58"))
ISpLexicon : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetPronunciations (
        /*[in]*/ LPWSTR pszWord,
        /*[in]*/ unsigned short LangId,
        /*[in]*/ unsigned long dwFlags,
        /*[in,out]*/ struct SPWORDPRONUNCIATIONLIST * pWordPronunciationList ) = 0;
      virtual HRESULT __stdcall AddPronunciation (
        /*[in]*/ LPWSTR pszWord,
        /*[in]*/ unsigned short LangId,
        /*[in]*/ enum SPPARTOFSPEECH ePartOfSpeech,
        /*[in]*/ LPWSTR pszPronunciation ) = 0;
      virtual HRESULT __stdcall RemovePronunciation (
        /*[in]*/ LPWSTR pszWord,
        /*[in]*/ unsigned short LangId,
        /*[in]*/ enum SPPARTOFSPEECH ePartOfSpeech,
        /*[in]*/ LPWSTR pszPronunciation ) = 0;
      virtual HRESULT __stdcall GetGeneration (
        unsigned long * pdwGeneration ) = 0;
      virtual HRESULT __stdcall GetGenerationChange (
        /*[in]*/ unsigned long dwFlags,
        /*[in,out]*/ unsigned long * pdwGeneration,
        /*[in,out]*/ struct SPWORDLIST * pWordList ) = 0;
      virtual HRESULT __stdcall GetWords (
        /*[in]*/ unsigned long dwFlags,
        /*[in,out]*/ unsigned long * pdwGeneration,
        /*[in,out]*/ unsigned long * pdwCookie,
        /*[in,out]*/ struct SPWORDLIST * pWordList ) = 0;
};

struct __declspec(uuid("c9e37c15-df92-4727-85d6-72e5eeb6995a"))
SpUnCompressedLexicon;
    // [ default ] interface ISpeechLexicon
    // interface ISpLexicon
    // interface ISpObjectWithToken
    // interface ISpPhoneticAlphabetSelection

struct __declspec(uuid("90903716-2f42-11d3-9c26-00c04f8ef87c"))
SpCompressedLexicon;
    // [ default ] interface ISpLexicon
    // interface ISpObjectWithToken

struct __declspec(uuid("0d722f1a-9fcf-4e62-96d8-6df8f01a26aa"))
SpShortcut;
    // [ default ] interface ISpShortcut
    // interface ISpObjectWithToken

enum SPSHORTCUTTYPE
{
    SPSHT_NotOverriden = -1,
    SPSHT_Unknown = 0,
    SPSHT_EMAIL = 4096,
    SPSHT_OTHER = 8192,
    SPPS_RESERVED1 = 12288,
    SPPS_RESERVED2 = 16384,
    SPPS_RESERVED3 = 20480,
    SPPS_RESERVED4 = 61440
};

#pragma pack(push, 4)

struct SPSHORTCUTPAIR
{
    struct SPSHORTCUTPAIR * pNextSHORTCUTPAIR;
    unsigned short LangId;
    enum SPSHORTCUTTYPE shType;
    LPWSTR pszDisplay;
    LPWSTR pszSpoken;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPSHORTCUTPAIRLIST
{
    unsigned long ulSize;
    unsigned char * pvBuffer;
    struct SPSHORTCUTPAIR * pFirstShortcutPair;
};

#pragma pack(pop)

struct __declspec(uuid("3df681e2-ea56-11d9-8bde-f66bad1e3f3a"))
ISpShortcut : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall AddShortcut (
        /*[in]*/ LPWSTR pszDisplay,
        /*[in]*/ unsigned short LangId,
        /*[in]*/ LPWSTR pszSpoken,
        /*[in]*/ enum SPSHORTCUTTYPE shType ) = 0;
      virtual HRESULT __stdcall RemoveShortcut (
        /*[in]*/ LPWSTR pszDisplay,
        /*[in]*/ unsigned short LangId,
        /*[in]*/ LPWSTR pszSpoken,
        /*[in]*/ enum SPSHORTCUTTYPE shType ) = 0;
      virtual HRESULT __stdcall GetShortcuts (
        /*[in]*/ unsigned short LangId,
        /*[in,out]*/ struct SPSHORTCUTPAIRLIST * pShortcutpairList ) = 0;
      virtual HRESULT __stdcall GetGeneration (
        unsigned long * pdwGeneration ) = 0;
      virtual HRESULT __stdcall GetWordsFromGenerationChange (
        /*[in,out]*/ unsigned long * pdwGeneration,
        /*[in,out]*/ struct SPWORDLIST * pWordList ) = 0;
      virtual HRESULT __stdcall GetWords (
        /*[in,out]*/ unsigned long * pdwGeneration,
        /*[in,out]*/ unsigned long * pdwCookie,
        /*[in,out]*/ struct SPWORDLIST * pWordList ) = 0;
      virtual HRESULT __stdcall GetShortcutsForGeneration (
        /*[in,out]*/ unsigned long * pdwGeneration,
        /*[in,out]*/ unsigned long * pdwCookie,
        /*[in,out]*/ struct SPSHORTCUTPAIRLIST * pShortcutpairList ) = 0;
      virtual HRESULT __stdcall GetGenerationChange (
        /*[in,out]*/ unsigned long * pdwGeneration,
        /*[in,out]*/ struct SPSHORTCUTPAIRLIST * pShortcutpairList ) = 0;
};

struct __declspec(uuid("9185f743-1143-4c28-86b5-bff14f20e5c8"))
SpPhoneConverter;
    // [ default ] interface ISpeechPhoneConverter
    // interface ISpPhoneConverter
    // interface ISpPhoneticAlphabetSelection

struct __declspec(uuid("4f414126-dfe3-4629-99ee-797978317ead"))
SpPhoneticAlphabetConverter;
    // [ default ] interface ISpPhoneticAlphabetConverter

struct __declspec(uuid("133adcd4-19b4-4020-9fdc-842e78253b17"))
ISpPhoneticAlphabetConverter : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetLangId (
        /*[out]*/ unsigned short * pLangID ) = 0;
      virtual HRESULT __stdcall SetLangId (
        unsigned short LangId ) = 0;
      virtual HRESULT __stdcall SAPI2UPS (
        /*[in]*/ unsigned short * pszSAPIId,
        /*[out]*/ unsigned short * pszUPSId,
        unsigned long cMaxLength ) = 0;
      virtual HRESULT __stdcall UPS2SAPI (
        /*[in]*/ unsigned short * pszUPSId,
        /*[out]*/ unsigned short * pszSAPIId,
        unsigned long cMaxLength ) = 0;
      virtual HRESULT __stdcall GetMaxConvertLength (
        unsigned long cSrcLength,
        long bSAPI2UPS,
        /*[out]*/ unsigned long * pcMaxDestLength ) = 0;
};

struct __declspec(uuid("455f24e9-7396-4a16-9715-7c0fdbe3efe3"))
SpNullPhoneConverter;
    // [ default ] interface ISpPhoneConverter

struct __declspec(uuid("0f92030a-cbfd-4ab8-a164-ff5985547ff6"))
SpTextSelectionInformation;
    // [ default ] interface ISpeechTextSelectionInformation

struct __declspec(uuid("c23fc28d-c55f-4720-8b32-91f73c2bd5d1"))
SpPhraseInfoBuilder;
    // [ default ] interface ISpeechPhraseInfoBuilder

struct __declspec(uuid("9ef96870-e160-4792-820d-48cf0649e4ec"))
SpAudioFormat;
    // [ default ] interface ISpeechAudioFormat

struct __declspec(uuid("c79a574c-63be-44b9-801f-283f87f898be"))
SpWaveFormatEx;
    // [ default ] interface ISpeechWaveFormatEx

struct __declspec(uuid("73ad6842-ace0-45e8-a4dd-8795881a2c2a"))
SpInProcRecoContext;
    // [ default ] interface ISpeechRecoContext
    // interface ISpRecoContext
    // interface ISpRecoContext2
    // interface ISpPhoneticAlphabetSelection
    // [ default, source ] dispinterface _ISpeechRecoContextEvents

struct __declspec(uuid("8dbef13f-1948-4aa8-8cf0-048eebed95d8"))
SpCustomStream;
    // [ default ] interface ISpeechCustomStream
    // interface ISpStream

struct __declspec(uuid("947812b3-2ae1-4644-ba86-9e90ded7ec91"))
SpFileStream;
    // [ default ] interface ISpeechFileStream
    // interface ISpStream

struct __declspec(uuid("5fb7ef7d-dff4-468a-b6b7-2fcbd188f994"))
SpMemoryStream;
    // [ default ] interface ISpeechMemoryStream
    // interface ISpStream

#pragma pack(push, 4)

struct SPSEMANTICERRORINFO
{
    unsigned long ulLineNumber;
    LPWSTR pszScriptLine;
    LPWSTR pszSource;
    LPWSTR pszDescription;
    HRESULT hrResultCode;
};

#pragma pack(pop)

#pragma pack(push, 4)

struct SPRULE
{
    LPWSTR pszRuleName;
    unsigned long ulRuleId;
    unsigned long dwAttributes;
};

#pragma pack(pop)

struct __declspec(uuid("b9ac5783-fcd0-4b21-b119-b4f8da8fd2c3"))
ISpeechResourceLoader : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall LoadResource (
        /*[in]*/ BSTR bstrResourceUri,
        /*[in]*/ VARIANT_BOOL fAlwaysReload,
        /*[out]*/ IUnknown * * pStream,
        /*[out]*/ BSTR * pbstrMIMEType,
        /*[out]*/ VARIANT_BOOL * pfModified,
        /*[out]*/ BSTR * pbstrRedirectUrl ) = 0;
      virtual HRESULT __stdcall GetLocalCopy (
        /*[in]*/ BSTR bstrResourceUri,
        /*[out]*/ BSTR * pbstrLocalPath,
        /*[out]*/ BSTR * pbstrMIMEType,
        /*[out]*/ BSTR * pbstrRedirectUrl ) = 0;
      virtual HRESULT __stdcall ReleaseLocalCopy (
        /*[in]*/ BSTR pbstrLocalPath ) = 0;
};

struct __declspec(uuid("79eac9ed-baf9-11ce-8c82-00aa004ba90b"))
IInternetSecurityMgrSite : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetWindow (
        /*[out]*/ wireHWND * phwnd ) = 0;
      virtual HRESULT __stdcall EnableModeless (
        /*[in]*/ long fEnable ) = 0;
};

struct __declspec(uuid("79eac9ee-baf9-11ce-8c82-00aa004ba90b"))
IInternetSecurityManager : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetSecuritySite (
        /*[in]*/ struct IInternetSecurityMgrSite * pSite ) = 0;
      virtual HRESULT __stdcall GetSecuritySite (
        /*[out]*/ struct IInternetSecurityMgrSite * * ppSite ) = 0;
      virtual HRESULT __stdcall MapUrlToZone (
        /*[in]*/ LPWSTR pwszUrl,
        /*[out]*/ unsigned long * pdwZone,
        /*[in]*/ unsigned long dwFlags ) = 0;
      virtual HRESULT __stdcall GetSecurityId (
        /*[in]*/ LPWSTR pwszUrl,
        /*[out]*/ unsigned char * pbSecurityId,
        /*[in,out]*/ unsigned long * pcbSecurityId,
        /*[in]*/ ULONG_PTR dwReserved ) = 0;
      virtual HRESULT __stdcall ProcessUrlAction (
        /*[in]*/ LPWSTR pwszUrl,
        /*[in]*/ unsigned long dwAction,
        /*[out]*/ unsigned char * pPolicy,
        /*[in]*/ unsigned long cbPolicy,
        /*[in]*/ unsigned char * pContext,
        /*[in]*/ unsigned long cbContext,
        /*[in]*/ unsigned long dwFlags,
        /*[in]*/ unsigned long dwReserved ) = 0;
      virtual HRESULT __stdcall QueryCustomPolicy (
        /*[in]*/ LPWSTR pwszUrl,
        /*[in]*/ GUID * guidKey,
        /*[out]*/ unsigned char * * ppPolicy,
        /*[out]*/ unsigned long * pcbPolicy,
        /*[in]*/ unsigned char * pContext,
        /*[in]*/ unsigned long cbContext,
        /*[in]*/ unsigned long dwReserved ) = 0;
      virtual HRESULT __stdcall SetZoneMapping (
        /*[in]*/ unsigned long dwZone,
        /*[in]*/ LPWSTR lpszPattern,
        /*[in]*/ unsigned long dwFlags ) = 0;
      virtual HRESULT __stdcall GetZoneMappings (
        /*[in]*/ unsigned long dwZone,
        /*[out]*/ struct IEnumString * * ppenumString,
        /*[in]*/ unsigned long dwFlags ) = 0;
};

struct __declspec(uuid("4b37bc9e-9ed6-44a3-93d3-18f022b79ec3"))
ISpRecoGrammar2 : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetRules (
        /*[out]*/ struct SPRULE * * ppCoMemRules,
        /*[out]*/ unsigned int * puNumRules ) = 0;
      virtual HRESULT __stdcall LoadCmdFromFile2 (
        /*[in]*/ LPWSTR pszFileName,
        /*[in]*/ enum SPLOADOPTIONS Options,
        /*[in]*/ LPWSTR pszSharingUri,
        /*[in]*/ LPWSTR pszBaseUri ) = 0;
      virtual HRESULT __stdcall LoadCmdFromMemory2 (
        /*[in]*/ struct SPBINARYGRAMMAR * pGrammar,
        /*[in]*/ enum SPLOADOPTIONS Options,
        /*[in]*/ LPWSTR pszSharingUri,
        /*[in]*/ LPWSTR pszBaseUri ) = 0;
      virtual HRESULT __stdcall SetRulePriority (
        /*[in]*/ LPWSTR pszRuleName,
        /*[in]*/ unsigned long ulRuleId,
        /*[in]*/ int nRulePriority ) = 0;
      virtual HRESULT __stdcall SetRuleWeight (
        /*[in]*/ LPWSTR pszRuleName,
        /*[in]*/ unsigned long ulRuleId,
        /*[in]*/ float flWeight ) = 0;
      virtual HRESULT __stdcall SetDictationWeight (
        /*[in]*/ float flWeight ) = 0;
      virtual HRESULT __stdcall SetGrammarLoader (
        /*[in]*/ struct ISpeechResourceLoader * pLoader ) = 0;
      virtual HRESULT __stdcall SetSMLSecurityManager (
        /*[in]*/ struct IInternetSecurityManager * pSMLSecurityManager ) = 0;
};

struct __declspec(uuid("c74a3adc-b727-4500-a84a-b526721c8b8c"))
ISpeechObjectToken : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Id (
        /*[out,retval]*/ BSTR * ObjectId ) = 0;
      virtual HRESULT __stdcall get_DataKey (
        /*[out,retval]*/ struct ISpeechDataKey * * DataKey ) = 0;
      virtual HRESULT __stdcall get_Category (
        /*[out,retval]*/ struct ISpeechObjectTokenCategory * * Category ) = 0;
      virtual HRESULT __stdcall GetDescription (
        /*[in]*/ long Locale,
        /*[out,retval]*/ BSTR * Description ) = 0;
      virtual HRESULT __stdcall SetId (
        /*[in]*/ BSTR Id,
        /*[in]*/ BSTR CategoryID,
        /*[in]*/ VARIANT_BOOL CreateIfNotExist ) = 0;
      virtual HRESULT __stdcall GetAttribute (
        /*[in]*/ BSTR AttributeName,
        /*[out,retval]*/ BSTR * AttributeValue ) = 0;
      virtual HRESULT __stdcall CreateInstance (
        /*[in]*/ IUnknown * pUnkOuter,
        /*[in]*/ enum SpeechTokenContext ClsContext,
        /*[out,retval]*/ IUnknown * * Object ) = 0;
      virtual HRESULT __stdcall Remove (
        /*[in]*/ BSTR ObjectStorageCLSID ) = 0;
      virtual HRESULT __stdcall GetStorageFileName (
        /*[in]*/ BSTR ObjectStorageCLSID,
        /*[in]*/ BSTR KeyName,
        /*[in]*/ BSTR FileName,
        /*[in]*/ enum SpeechTokenShellFolder Folder,
        /*[out,retval]*/ BSTR * FilePath ) = 0;
      virtual HRESULT __stdcall RemoveStorageFileName (
        /*[in]*/ BSTR ObjectStorageCLSID,
        /*[in]*/ BSTR KeyName,
        /*[in]*/ VARIANT_BOOL DeleteFile ) = 0;
      virtual HRESULT __stdcall IsUISupported (
        /*[in]*/ BSTR TypeOfUI,
        /*[in]*/ VARIANT * ExtraData,
        /*[in]*/ IUnknown * Object,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall DisplayUI (
        /*[in]*/ long hWnd,
        /*[in]*/ BSTR Title,
        /*[in]*/ BSTR TypeOfUI,
        /*[in]*/ VARIANT * ExtraData,
        /*[in]*/ IUnknown * Object ) = 0;
      virtual HRESULT __stdcall MatchesAttributes (
        /*[in]*/ BSTR Attributes,
        /*[out,retval]*/ VARIANT_BOOL * Matches ) = 0;
};

struct __declspec(uuid("9285b776-2e7b-4bc0-b53e-580eb6fa967f"))
ISpeechObjectTokens : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechObjectToken * * Token ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * ppEnumVARIANT ) = 0;
};

struct __declspec(uuid("ca7eac50-2d01-4145-86d4-5ae7d70f4469"))
ISpeechObjectTokenCategory : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Id (
        /*[out,retval]*/ BSTR * Id ) = 0;
      virtual HRESULT __stdcall put_Default (
        /*[in]*/ BSTR TokenId ) = 0;
      virtual HRESULT __stdcall get_Default (
        /*[out,retval]*/ BSTR * TokenId ) = 0;
      virtual HRESULT __stdcall SetId (
        /*[in]*/ BSTR Id,
        /*[in]*/ VARIANT_BOOL CreateIfNotExist ) = 0;
      virtual HRESULT __stdcall GetDataKey (
        /*[in]*/ enum SpeechDataKeyLocation Location,
        /*[out,retval]*/ struct ISpeechDataKey * * DataKey ) = 0;
      virtual HRESULT __stdcall EnumerateTokens (
        /*[in]*/ BSTR RequiredAttributes,
        /*[in]*/ BSTR OptionalAttributes,
        /*[out,retval]*/ struct ISpeechObjectTokens * * Tokens ) = 0;
};

struct __declspec(uuid("269316d8-57bd-11d2-9eee-00c04f797396"))
ISpeechVoice : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Status (
        /*[out,retval]*/ struct ISpeechVoiceStatus * * Status ) = 0;
      virtual HRESULT __stdcall get_Voice (
        /*[out,retval]*/ struct ISpeechObjectToken * * Voice ) = 0;
      virtual HRESULT __stdcall putref_Voice (
        /*[in]*/ struct ISpeechObjectToken * Voice ) = 0;
      virtual HRESULT __stdcall get_AudioOutput (
        /*[out,retval]*/ struct ISpeechObjectToken * * AudioOutput ) = 0;
      virtual HRESULT __stdcall putref_AudioOutput (
        /*[in]*/ struct ISpeechObjectToken * AudioOutput ) = 0;
      virtual HRESULT __stdcall get_AudioOutputStream (
        /*[out,retval]*/ struct ISpeechBaseStream * * AudioOutputStream ) = 0;
      virtual HRESULT __stdcall putref_AudioOutputStream (
        /*[in]*/ struct ISpeechBaseStream * AudioOutputStream ) = 0;
      virtual HRESULT __stdcall get_Rate (
        /*[out,retval]*/ long * Rate ) = 0;
      virtual HRESULT __stdcall put_Rate (
        /*[in]*/ long Rate ) = 0;
      virtual HRESULT __stdcall get_Volume (
        /*[out,retval]*/ long * Volume ) = 0;
      virtual HRESULT __stdcall put_Volume (
        /*[in]*/ long Volume ) = 0;
      virtual HRESULT __stdcall put_AllowAudioOutputFormatChangesOnNextSet (
        /*[in]*/ VARIANT_BOOL Allow ) = 0;
      virtual HRESULT __stdcall get_AllowAudioOutputFormatChangesOnNextSet (
        /*[out,retval]*/ VARIANT_BOOL * Allow ) = 0;
      virtual HRESULT __stdcall get_EventInterests (
        /*[out,retval]*/ enum SpeechVoiceEvents * EventInterestFlags ) = 0;
      virtual HRESULT __stdcall put_EventInterests (
        /*[in]*/ enum SpeechVoiceEvents EventInterestFlags ) = 0;
      virtual HRESULT __stdcall put_Priority (
        /*[in]*/ enum SpeechVoicePriority Priority ) = 0;
      virtual HRESULT __stdcall get_Priority (
        /*[out,retval]*/ enum SpeechVoicePriority * Priority ) = 0;
      virtual HRESULT __stdcall put_AlertBoundary (
        /*[in]*/ enum SpeechVoiceEvents Boundary ) = 0;
      virtual HRESULT __stdcall get_AlertBoundary (
        /*[out,retval]*/ enum SpeechVoiceEvents * Boundary ) = 0;
      virtual HRESULT __stdcall put_SynchronousSpeakTimeout (
        /*[in]*/ long msTimeout ) = 0;
      virtual HRESULT __stdcall get_SynchronousSpeakTimeout (
        /*[out,retval]*/ long * msTimeout ) = 0;
      virtual HRESULT __stdcall Speak (
        /*[in]*/ BSTR Text,
        /*[in]*/ enum SpeechVoiceSpeakFlags Flags,
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall SpeakStream (
        /*[in]*/ struct ISpeechBaseStream * Stream,
        /*[in]*/ enum SpeechVoiceSpeakFlags Flags,
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall Pause ( ) = 0;
      virtual HRESULT __stdcall Resume ( ) = 0;
      virtual HRESULT __stdcall Skip (
        /*[in]*/ BSTR Type,
        /*[in]*/ long NumItems,
        /*[out,retval]*/ long * NumSkipped ) = 0;
      virtual HRESULT __stdcall GetVoices (
        /*[in]*/ BSTR RequiredAttributes,
        /*[in]*/ BSTR OptionalAttributes,
        /*[out,retval]*/ struct ISpeechObjectTokens * * ObjectTokens ) = 0;
      virtual HRESULT __stdcall GetAudioOutputs (
        /*[in]*/ BSTR RequiredAttributes,
        /*[in]*/ BSTR OptionalAttributes,
        /*[out,retval]*/ struct ISpeechObjectTokens * * ObjectTokens ) = 0;
      virtual HRESULT __stdcall WaitUntilDone (
        /*[in]*/ long msTimeout,
        /*[out,retval]*/ VARIANT_BOOL * Done ) = 0;
      virtual HRESULT __stdcall SpeakCompleteEvent (
        /*[out,retval]*/ long * Handle ) = 0;
      virtual HRESULT __stdcall IsUISupported (
        /*[in]*/ BSTR TypeOfUI,
        /*[in]*/ VARIANT * ExtraData,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall DisplayUI (
        /*[in]*/ long hWndParent,
        /*[in]*/ BSTR Title,
        /*[in]*/ BSTR TypeOfUI,
        /*[in]*/ VARIANT * ExtraData ) = 0;
};

struct __declspec(uuid("2d5f1c0c-bd75-4b08-9478-3b11fea2586c"))
ISpeechRecognizer : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall putref_Recognizer (
        /*[in]*/ struct ISpeechObjectToken * Recognizer ) = 0;
      virtual HRESULT __stdcall get_Recognizer (
        /*[out,retval]*/ struct ISpeechObjectToken * * Recognizer ) = 0;
      virtual HRESULT __stdcall put_AllowAudioInputFormatChangesOnNextSet (
        /*[in]*/ VARIANT_BOOL Allow ) = 0;
      virtual HRESULT __stdcall get_AllowAudioInputFormatChangesOnNextSet (
        /*[out,retval]*/ VARIANT_BOOL * Allow ) = 0;
      virtual HRESULT __stdcall putref_AudioInput (
        /*[in]*/ struct ISpeechObjectToken * AudioInput ) = 0;
      virtual HRESULT __stdcall get_AudioInput (
        /*[out,retval]*/ struct ISpeechObjectToken * * AudioInput ) = 0;
      virtual HRESULT __stdcall putref_AudioInputStream (
        /*[in]*/ struct ISpeechBaseStream * AudioInputStream ) = 0;
      virtual HRESULT __stdcall get_AudioInputStream (
        /*[out,retval]*/ struct ISpeechBaseStream * * AudioInputStream ) = 0;
      virtual HRESULT __stdcall get_IsShared (
        /*[out,retval]*/ VARIANT_BOOL * Shared ) = 0;
      virtual HRESULT __stdcall put_State (
        /*[in]*/ enum SpeechRecognizerState State ) = 0;
      virtual HRESULT __stdcall get_State (
        /*[out,retval]*/ enum SpeechRecognizerState * State ) = 0;
      virtual HRESULT __stdcall get_Status (
        /*[out,retval]*/ struct ISpeechRecognizerStatus * * Status ) = 0;
      virtual HRESULT __stdcall putref_Profile (
        /*[in]*/ struct ISpeechObjectToken * Profile ) = 0;
      virtual HRESULT __stdcall get_Profile (
        /*[out,retval]*/ struct ISpeechObjectToken * * Profile ) = 0;
      virtual HRESULT __stdcall EmulateRecognition (
        /*[in]*/ VARIANT TextElements,
        /*[in]*/ VARIANT * ElementDisplayAttributes,
        /*[in]*/ long LanguageId ) = 0;
      virtual HRESULT __stdcall CreateRecoContext (
        /*[out,retval]*/ struct ISpeechRecoContext * * NewContext ) = 0;
      virtual HRESULT __stdcall GetFormat (
        /*[in]*/ enum SpeechFormatType Type,
        /*[out,retval]*/ struct ISpeechAudioFormat * * Format ) = 0;
      virtual HRESULT __stdcall SetPropertyNumber (
        /*[in]*/ BSTR Name,
        /*[in]*/ long Value,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall GetPropertyNumber (
        /*[in]*/ BSTR Name,
        /*[in,out]*/ long * Value,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall SetPropertyString (
        /*[in]*/ BSTR Name,
        /*[in]*/ BSTR Value,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall GetPropertyString (
        /*[in]*/ BSTR Name,
        /*[in,out]*/ BSTR * Value,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall IsUISupported (
        /*[in]*/ BSTR TypeOfUI,
        /*[in]*/ VARIANT * ExtraData,
        /*[out,retval]*/ VARIANT_BOOL * Supported ) = 0;
      virtual HRESULT __stdcall DisplayUI (
        /*[in]*/ long hWndParent,
        /*[in]*/ BSTR Title,
        /*[in]*/ BSTR TypeOfUI,
        /*[in]*/ VARIANT * ExtraData ) = 0;
      virtual HRESULT __stdcall GetRecognizers (
        /*[in]*/ BSTR RequiredAttributes,
        /*[in]*/ BSTR OptionalAttributes,
        /*[out,retval]*/ struct ISpeechObjectTokens * * ObjectTokens ) = 0;
      virtual HRESULT __stdcall GetAudioInputs (
        /*[in]*/ BSTR RequiredAttributes,
        /*[in]*/ BSTR OptionalAttributes,
        /*[out,retval]*/ struct ISpeechObjectTokens * * ObjectTokens ) = 0;
      virtual HRESULT __stdcall GetProfiles (
        /*[in]*/ BSTR RequiredAttributes,
        /*[in]*/ BSTR OptionalAttributes,
        /*[out,retval]*/ struct ISpeechObjectTokens * * ObjectTokens ) = 0;
};

struct __declspec(uuid("580aa49d-7e1e-4809-b8e2-57da806104b8"))
ISpeechRecoContext : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Recognizer (
        /*[out,retval]*/ struct ISpeechRecognizer * * Recognizer ) = 0;
      virtual HRESULT __stdcall get_AudioInputInterferenceStatus (
        /*[out,retval]*/ enum SpeechInterference * Interference ) = 0;
      virtual HRESULT __stdcall get_RequestedUIType (
        /*[out,retval]*/ BSTR * UIType ) = 0;
      virtual HRESULT __stdcall putref_Voice (
        /*[in]*/ struct ISpeechVoice * Voice ) = 0;
      virtual HRESULT __stdcall get_Voice (
        /*[out,retval]*/ struct ISpeechVoice * * Voice ) = 0;
      virtual HRESULT __stdcall put_AllowVoiceFormatMatchingOnNextSet (
        /*[in]*/ VARIANT_BOOL pAllow ) = 0;
      virtual HRESULT __stdcall get_AllowVoiceFormatMatchingOnNextSet (
        /*[out,retval]*/ VARIANT_BOOL * pAllow ) = 0;
      virtual HRESULT __stdcall put_VoicePurgeEvent (
        /*[in]*/ enum SpeechRecoEvents EventInterest ) = 0;
      virtual HRESULT __stdcall get_VoicePurgeEvent (
        /*[out,retval]*/ enum SpeechRecoEvents * EventInterest ) = 0;
      virtual HRESULT __stdcall put_EventInterests (
        /*[in]*/ enum SpeechRecoEvents EventInterest ) = 0;
      virtual HRESULT __stdcall get_EventInterests (
        /*[out,retval]*/ enum SpeechRecoEvents * EventInterest ) = 0;
      virtual HRESULT __stdcall put_CmdMaxAlternates (
        /*[in]*/ long MaxAlternates ) = 0;
      virtual HRESULT __stdcall get_CmdMaxAlternates (
        /*[out,retval]*/ long * MaxAlternates ) = 0;
      virtual HRESULT __stdcall put_State (
        /*[in]*/ enum SpeechRecoContextState State ) = 0;
      virtual HRESULT __stdcall get_State (
        /*[out,retval]*/ enum SpeechRecoContextState * State ) = 0;
      virtual HRESULT __stdcall put_RetainedAudio (
        /*[in]*/ enum SpeechRetainedAudioOptions Option ) = 0;
      virtual HRESULT __stdcall get_RetainedAudio (
        /*[out,retval]*/ enum SpeechRetainedAudioOptions * Option ) = 0;
      virtual HRESULT __stdcall putref_RetainedAudioFormat (
        /*[in]*/ struct ISpeechAudioFormat * Format ) = 0;
      virtual HRESULT __stdcall get_RetainedAudioFormat (
        /*[out,retval]*/ struct ISpeechAudioFormat * * Format ) = 0;
      virtual HRESULT __stdcall Pause ( ) = 0;
      virtual HRESULT __stdcall Resume ( ) = 0;
      virtual HRESULT __stdcall CreateGrammar (
        /*[in]*/ VARIANT GrammarId,
        /*[out,retval]*/ struct ISpeechRecoGrammar * * Grammar ) = 0;
      virtual HRESULT __stdcall CreateResultFromMemory (
        /*[in]*/ VARIANT * ResultBlock,
        /*[out,retval]*/ struct ISpeechRecoResult * * Result ) = 0;
      virtual HRESULT __stdcall Bookmark (
        /*[in]*/ enum SpeechBookmarkOptions Options,
        /*[in]*/ VARIANT StreamPos,
        /*[in]*/ VARIANT BookmarkId ) = 0;
      virtual HRESULT __stdcall SetAdaptationData (
        /*[in]*/ BSTR AdaptationString ) = 0;
};

struct __declspec(uuid("b6d6f79f-2158-4e50-b5bc-9a9ccd852a09"))
ISpeechRecoGrammar : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Id (
        /*[out,retval]*/ VARIANT * Id ) = 0;
      virtual HRESULT __stdcall get_RecoContext (
        /*[out,retval]*/ struct ISpeechRecoContext * * RecoContext ) = 0;
      virtual HRESULT __stdcall put_State (
        /*[in]*/ enum SpeechGrammarState State ) = 0;
      virtual HRESULT __stdcall get_State (
        /*[out,retval]*/ enum SpeechGrammarState * State ) = 0;
      virtual HRESULT __stdcall get_Rules (
        /*[out,retval]*/ struct ISpeechGrammarRules * * Rules ) = 0;
      virtual HRESULT __stdcall Reset (
        /*[in]*/ long NewLanguage ) = 0;
      virtual HRESULT __stdcall CmdLoadFromFile (
        /*[in]*/ BSTR FileName,
        /*[in]*/ enum SpeechLoadOption LoadOption ) = 0;
      virtual HRESULT __stdcall CmdLoadFromObject (
        /*[in]*/ BSTR ClassId,
        /*[in]*/ BSTR GrammarName,
        /*[in]*/ enum SpeechLoadOption LoadOption ) = 0;
      virtual HRESULT __stdcall CmdLoadFromResource (
        /*[in]*/ long hModule,
        /*[in]*/ VARIANT ResourceName,
        /*[in]*/ VARIANT ResourceType,
        /*[in]*/ long LanguageId,
        /*[in]*/ enum SpeechLoadOption LoadOption ) = 0;
      virtual HRESULT __stdcall CmdLoadFromMemory (
        /*[in]*/ VARIANT GrammarData,
        /*[in]*/ enum SpeechLoadOption LoadOption ) = 0;
      virtual HRESULT __stdcall CmdLoadFromProprietaryGrammar (
        /*[in]*/ BSTR ProprietaryGuid,
        /*[in]*/ BSTR ProprietaryString,
        /*[in]*/ VARIANT ProprietaryData,
        /*[in]*/ enum SpeechLoadOption LoadOption ) = 0;
      virtual HRESULT __stdcall CmdSetRuleState (
        /*[in]*/ BSTR Name,
        /*[in]*/ enum SpeechRuleState State ) = 0;
      virtual HRESULT __stdcall CmdSetRuleIdState (
        /*[in]*/ long RuleId,
        /*[in]*/ enum SpeechRuleState State ) = 0;
      virtual HRESULT __stdcall DictationLoad (
        /*[in]*/ BSTR TopicName,
        /*[in]*/ enum SpeechLoadOption LoadOption ) = 0;
      virtual HRESULT __stdcall DictationUnload ( ) = 0;
      virtual HRESULT __stdcall DictationSetState (
        /*[in]*/ enum SpeechRuleState State ) = 0;
      virtual HRESULT __stdcall SetWordSequenceData (
        /*[in]*/ BSTR Text,
        /*[in]*/ long TextLength,
        /*[in]*/ struct ISpeechTextSelectionInformation * Info ) = 0;
      virtual HRESULT __stdcall SetTextSelection (
        /*[in]*/ struct ISpeechTextSelectionInformation * Info ) = 0;
      virtual HRESULT __stdcall IsPronounceable (
        /*[in]*/ BSTR Word,
        /*[out,retval]*/ enum SpeechWordPronounceable * WordPronounceable ) = 0;
};

struct __declspec(uuid("6ffa3b44-fc2d-40d1-8afc-32911c7f1ad1"))
ISpeechGrammarRules : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall FindRule (
        /*[in]*/ VARIANT RuleNameOrId,
        /*[out,retval]*/ struct ISpeechGrammarRule * * Rule ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechGrammarRule * * Rule ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
      virtual HRESULT __stdcall get_Dynamic (
        /*[out,retval]*/ VARIANT_BOOL * Dynamic ) = 0;
      virtual HRESULT __stdcall Add (
        /*[in]*/ BSTR RuleName,
        /*[in]*/ enum SpeechRuleAttributes Attributes,
        /*[in]*/ long RuleId,
        /*[out,retval]*/ struct ISpeechGrammarRule * * Rule ) = 0;
      virtual HRESULT __stdcall Commit ( ) = 0;
      virtual HRESULT __stdcall CommitAndSave (
        /*[out]*/ BSTR * ErrorText,
        /*[out,retval]*/ VARIANT * SaveStream ) = 0;
};

struct __declspec(uuid("afe719cf-5dd1-44f2-999c-7a399f1cfccc"))
ISpeechGrammarRule : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Attributes (
        /*[out,retval]*/ enum SpeechRuleAttributes * Attributes ) = 0;
      virtual HRESULT __stdcall get_InitialState (
        /*[out,retval]*/ struct ISpeechGrammarRuleState * * State ) = 0;
      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Name ) = 0;
      virtual HRESULT __stdcall get_Id (
        /*[out,retval]*/ long * Id ) = 0;
      virtual HRESULT __stdcall Clear ( ) = 0;
      virtual HRESULT __stdcall AddResource (
        /*[in]*/ BSTR ResourceName,
        /*[in]*/ BSTR ResourceValue ) = 0;
      virtual HRESULT __stdcall AddState (
        /*[out,retval]*/ struct ISpeechGrammarRuleState * * State ) = 0;
};

struct __declspec(uuid("d4286f2c-ee67-45ae-b928-28d695362eda"))
ISpeechGrammarRuleState : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Rule (
        /*[out,retval]*/ struct ISpeechGrammarRule * * Rule ) = 0;
      virtual HRESULT __stdcall get_Transitions (
        /*[out,retval]*/ struct ISpeechGrammarRuleStateTransitions * * Transitions ) = 0;
      virtual HRESULT __stdcall AddWordTransition (
        /*[in]*/ struct ISpeechGrammarRuleState * DestState,
        /*[in]*/ BSTR Words,
        /*[in]*/ BSTR Separators,
        /*[in]*/ enum SpeechGrammarWordType Type,
        /*[in]*/ BSTR PropertyName,
        /*[in]*/ long PropertyId,
        /*[in]*/ VARIANT * PropertyValue,
        /*[in]*/ float Weight ) = 0;
      virtual HRESULT __stdcall AddRuleTransition (
        /*[in]*/ struct ISpeechGrammarRuleState * DestinationState,
        /*[in]*/ struct ISpeechGrammarRule * Rule,
        /*[in]*/ BSTR PropertyName,
        /*[in]*/ long PropertyId,
        /*[in]*/ VARIANT * PropertyValue,
        /*[in]*/ float Weight ) = 0;
      virtual HRESULT __stdcall AddSpecialTransition (
        /*[in]*/ struct ISpeechGrammarRuleState * DestinationState,
        /*[in]*/ enum SpeechSpecialTransitionType Type,
        /*[in]*/ BSTR PropertyName,
        /*[in]*/ long PropertyId,
        /*[in]*/ VARIANT * PropertyValue,
        /*[in]*/ float Weight ) = 0;
};

struct __declspec(uuid("cafd1db1-41d1-4a06-9863-e2e81da17a9a"))
ISpeechGrammarRuleStateTransition : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Type (
        /*[out,retval]*/ enum SpeechGrammarRuleStateTransitionType * Type ) = 0;
      virtual HRESULT __stdcall get_Text (
        /*[out,retval]*/ BSTR * Text ) = 0;
      virtual HRESULT __stdcall get_Rule (
        /*[out,retval]*/ struct ISpeechGrammarRule * * Rule ) = 0;
      virtual HRESULT __stdcall get_Weight (
        /*[out,retval]*/ VARIANT * Weight ) = 0;
      virtual HRESULT __stdcall get_PropertyName (
        /*[out,retval]*/ BSTR * PropertyName ) = 0;
      virtual HRESULT __stdcall get_PropertyId (
        /*[out,retval]*/ long * PropertyId ) = 0;
      virtual HRESULT __stdcall get_PropertyValue (
        /*[out,retval]*/ VARIANT * PropertyValue ) = 0;
      virtual HRESULT __stdcall get_NextState (
        /*[out,retval]*/ struct ISpeechGrammarRuleState * * NextState ) = 0;
};

struct __declspec(uuid("eabce657-75bc-44a2-aa7f-c56476742963"))
ISpeechGrammarRuleStateTransitions : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechGrammarRuleStateTransition * * Transition ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("ed2879cf-ced9-4ee6-a534-de0191d5468d"))
ISpeechRecoResult : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_RecoContext (
        /*[out,retval]*/ struct ISpeechRecoContext * * RecoContext ) = 0;
      virtual HRESULT __stdcall get_Times (
        /*[out,retval]*/ struct ISpeechRecoResultTimes * * Times ) = 0;
      virtual HRESULT __stdcall putref_AudioFormat (
        /*[in]*/ struct ISpeechAudioFormat * Format ) = 0;
      virtual HRESULT __stdcall get_AudioFormat (
        /*[out,retval]*/ struct ISpeechAudioFormat * * Format ) = 0;
      virtual HRESULT __stdcall get_PhraseInfo (
        /*[out,retval]*/ struct ISpeechPhraseInfo * * PhraseInfo ) = 0;
      virtual HRESULT __stdcall Alternates (
        /*[in]*/ long RequestCount,
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[out,retval]*/ struct ISpeechPhraseAlternates * * Alternates ) = 0;
      virtual HRESULT __stdcall Audio (
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[out,retval]*/ struct ISpeechMemoryStream * * Stream ) = 0;
      virtual HRESULT __stdcall SpeakAudio (
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[in]*/ enum SpeechVoiceSpeakFlags Flags,
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall SaveToMemory (
        /*[out,retval]*/ VARIANT * ResultBlock ) = 0;
      virtual HRESULT __stdcall DiscardResultInfo (
        /*[in]*/ enum SpeechDiscardType ValueTypes ) = 0;
};

struct __declspec(uuid("8e0a246d-d3c8-45de-8657-04290c458c3c"))
ISpeechRecoResult2 : ISpeechRecoResult
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetTextFeedback (
        /*[in]*/ BSTR Feedback,
        /*[in]*/ VARIANT_BOOL WasSuccessful ) = 0;
};

struct __declspec(uuid("aaec54af-8f85-4924-944d-b79d39d72e19"))
ISpeechXMLRecoResult : ISpeechRecoResult
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetXMLResult (
        /*[in]*/ enum SPXMLRESULTOPTIONS Options,
        /*[out,retval]*/ BSTR * pResult ) = 0;
      virtual HRESULT __stdcall GetXMLErrorInfo (
        /*[out]*/ long * LineNumber,
        /*[out]*/ BSTR * ScriptLine,
        /*[out]*/ BSTR * Source,
        /*[out]*/ BSTR * Description,
        /*[out]*/ long * ResultCode,
        /*[out,retval]*/ VARIANT_BOOL * IsError ) = 0;
};

struct __declspec(uuid("961559cf-4e67-4662-8bf0-d93f1fcd61b3"))
ISpeechPhraseInfo : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_LanguageId (
        /*[out,retval]*/ long * LanguageId ) = 0;
      virtual HRESULT __stdcall get_GrammarId (
        /*[out,retval]*/ VARIANT * GrammarId ) = 0;
      virtual HRESULT __stdcall get_StartTime (
        /*[out,retval]*/ VARIANT * StartTime ) = 0;
      virtual HRESULT __stdcall get_AudioStreamPosition (
        /*[out,retval]*/ VARIANT * AudioStreamPosition ) = 0;
      virtual HRESULT __stdcall get_AudioSizeBytes (
        /*[out,retval]*/ long * pAudioSizeBytes ) = 0;
      virtual HRESULT __stdcall get_RetainedSizeBytes (
        /*[out,retval]*/ long * RetainedSizeBytes ) = 0;
      virtual HRESULT __stdcall get_AudioSizeTime (
        /*[out,retval]*/ long * AudioSizeTime ) = 0;
      virtual HRESULT __stdcall get_Rule (
        /*[out,retval]*/ struct ISpeechPhraseRule * * Rule ) = 0;
      virtual HRESULT __stdcall get_Properties (
        /*[out,retval]*/ struct ISpeechPhraseProperties * * Properties ) = 0;
      virtual HRESULT __stdcall get_Elements (
        /*[out,retval]*/ struct ISpeechPhraseElements * * Elements ) = 0;
      virtual HRESULT __stdcall get_Replacements (
        /*[out,retval]*/ struct ISpeechPhraseReplacements * * Replacements ) = 0;
      virtual HRESULT __stdcall get_EngineId (
        /*[out,retval]*/ BSTR * EngineIdGuid ) = 0;
      virtual HRESULT __stdcall get_EnginePrivateData (
        /*[out,retval]*/ VARIANT * PrivateData ) = 0;
      virtual HRESULT __stdcall SaveToMemory (
        /*[out,retval]*/ VARIANT * PhraseBlock ) = 0;
      virtual HRESULT __stdcall GetText (
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[in]*/ VARIANT_BOOL UseReplacements,
        /*[out,retval]*/ BSTR * Text ) = 0;
      virtual HRESULT __stdcall GetDisplayAttributes (
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[in]*/ VARIANT_BOOL UseReplacements,
        /*[out,retval]*/ enum SpeechDisplayAttributes * DisplayAttributes ) = 0;
};

struct __declspec(uuid("27864a2a-2b9f-4cb8-92d3-0d2722fd1e73"))
ISpeechPhraseAlternate : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_RecoResult (
        /*[out,retval]*/ struct ISpeechRecoResult * * RecoResult ) = 0;
      virtual HRESULT __stdcall get_StartElementInResult (
        /*[out,retval]*/ long * StartElement ) = 0;
      virtual HRESULT __stdcall get_NumberOfElementsInResult (
        /*[out,retval]*/ long * NumberOfElements ) = 0;
      virtual HRESULT __stdcall get_PhraseInfo (
        /*[out,retval]*/ struct ISpeechPhraseInfo * * PhraseInfo ) = 0;
      virtual HRESULT __stdcall Commit ( ) = 0;
};

struct __declspec(uuid("b238b6d5-f276-4c3d-a6c1-2974801c3cc2"))
ISpeechPhraseAlternates : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechPhraseAlternate * * PhraseAlternate ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("6d60eb64-aced-40a6-bbf3-4e557f71dee2"))
ISpeechRecoResultDispatch : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_RecoContext (
        /*[out,retval]*/ struct ISpeechRecoContext * * RecoContext ) = 0;
      virtual HRESULT __stdcall get_Times (
        /*[out,retval]*/ struct ISpeechRecoResultTimes * * Times ) = 0;
      virtual HRESULT __stdcall putref_AudioFormat (
        /*[in]*/ struct ISpeechAudioFormat * Format ) = 0;
      virtual HRESULT __stdcall get_AudioFormat (
        /*[out,retval]*/ struct ISpeechAudioFormat * * Format ) = 0;
      virtual HRESULT __stdcall get_PhraseInfo (
        /*[out,retval]*/ struct ISpeechPhraseInfo * * PhraseInfo ) = 0;
      virtual HRESULT __stdcall Alternates (
        /*[in]*/ long RequestCount,
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[out,retval]*/ struct ISpeechPhraseAlternates * * Alternates ) = 0;
      virtual HRESULT __stdcall Audio (
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[out,retval]*/ struct ISpeechMemoryStream * * Stream ) = 0;
      virtual HRESULT __stdcall SpeakAudio (
        /*[in]*/ long StartElement,
        /*[in]*/ long Elements,
        /*[in]*/ enum SpeechVoiceSpeakFlags Flags,
        /*[out,retval]*/ long * StreamNumber ) = 0;
      virtual HRESULT __stdcall SaveToMemory (
        /*[out,retval]*/ VARIANT * ResultBlock ) = 0;
      virtual HRESULT __stdcall DiscardResultInfo (
        /*[in]*/ enum SpeechDiscardType ValueTypes ) = 0;
      virtual HRESULT __stdcall GetXMLResult (
        /*[in]*/ enum SPXMLRESULTOPTIONS Options,
        /*[out,retval]*/ BSTR * pResult ) = 0;
      virtual HRESULT __stdcall GetXMLErrorInfo (
        /*[out]*/ long * LineNumber,
        /*[out]*/ BSTR * ScriptLine,
        /*[out]*/ BSTR * Source,
        /*[out]*/ BSTR * Description,
        /*[out]*/ HRESULT * ResultCode,
        /*[out,retval]*/ VARIANT_BOOL * IsError ) = 0;
      virtual HRESULT __stdcall SetTextFeedback (
        /*[in]*/ BSTR Feedback,
        /*[in]*/ VARIANT_BOOL WasSuccessful ) = 0;
};

struct __declspec(uuid("3b151836-df3a-4e0a-846c-d2adc9334333"))
ISpeechPhraseInfoBuilder : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall RestorePhraseFromMemory (
        /*[in]*/ VARIANT * PhraseInMemory,
        /*[out,retval]*/ struct ISpeechPhraseInfo * * PhraseInfo ) = 0;
};

struct __declspec(uuid("a7bfe112-a4a0-48d9-b602-c313843f6964"))
ISpeechPhraseRule : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Name ) = 0;
      virtual HRESULT __stdcall get_Id (
        /*[out,retval]*/ long * Id ) = 0;
      virtual HRESULT __stdcall get_FirstElement (
        /*[out,retval]*/ long * FirstElement ) = 0;
      virtual HRESULT __stdcall get_NumberOfElements (
        /*[out,retval]*/ long * NumberOfElements ) = 0;
      virtual HRESULT __stdcall get_Parent (
        /*[out,retval]*/ struct ISpeechPhraseRule * * Parent ) = 0;
      virtual HRESULT __stdcall get_Children (
        /*[out,retval]*/ struct ISpeechPhraseRules * * Children ) = 0;
      virtual HRESULT __stdcall get_Confidence (
        /*[out,retval]*/ enum SpeechEngineConfidence * ActualConfidence ) = 0;
      virtual HRESULT __stdcall get_EngineConfidence (
        /*[out,retval]*/ float * EngineConfidence ) = 0;
};

struct __declspec(uuid("9047d593-01dd-4b72-81a3-e4a0ca69f407"))
ISpeechPhraseRules : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechPhraseRule * * Rule ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("08166b47-102e-4b23-a599-bdb98dbfd1f4"))
ISpeechPhraseProperties : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Count ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct ISpeechPhraseProperty * * Property ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * EnumVARIANT ) = 0;
};

struct __declspec(uuid("ce563d48-961e-4732-a2e1-378a42b430be"))
ISpeechPhraseProperty : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Name ) = 0;
      virtual HRESULT __stdcall get_Id (
        /*[out,retval]*/ long * Id ) = 0;
      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ VARIANT * Value ) = 0;
      virtual HRESULT __stdcall get_FirstElement (
        /*[out,retval]*/ long * FirstElement ) = 0;
      virtual HRESULT __stdcall get_NumberOfElements (
        /*[out,retval]*/ long * NumberOfElements ) = 0;
      virtual HRESULT __stdcall get_EngineConfidence (
        /*[out,retval]*/ float * Confidence ) = 0;
      virtual HRESULT __stdcall get_Confidence (
        /*[out,retval]*/ enum SpeechEngineConfidence * Confidence ) = 0;
      virtual HRESULT __stdcall get_Parent (
        /*[out,retval]*/ struct ISpeechPhraseProperty * * ParentProperty ) = 0;
      virtual HRESULT __stdcall get_Children (
        /*[out,retval]*/ struct ISpeechPhraseProperties * * Children ) = 0;
};

struct __declspec(uuid("2d3d3845-39af-4850-bbf9-40b49780011d"))
ISpObjectTokenCategory : ISpDataKey
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetId (
        /*[in]*/ LPWSTR pszCategoryId,
        long fCreateIfNotExist ) = 0;
      virtual HRESULT __stdcall GetId (
        /*[out]*/ LPWSTR * ppszCoMemCategoryId ) = 0;
      virtual HRESULT __stdcall GetDataKey (
        enum SPDATAKEYLOCATION spdkl,
        struct ISpDataKey * * ppDataKey ) = 0;
      virtual HRESULT __stdcall EnumTokens (
        /*[in]*/ LPWSTR pzsReqAttribs,
        /*[in]*/ LPWSTR pszOptAttribs,
        /*[out]*/ struct IEnumSpObjectTokens * * ppEnum ) = 0;
      virtual HRESULT __stdcall SetDefaultTokenId (
        /*[in]*/ LPWSTR pszTokenId ) = 0;
      virtual HRESULT __stdcall GetDefaultTokenId (
        /*[out]*/ LPWSTR * ppszCoMemTokenId ) = 0;
};

struct __declspec(uuid("14056589-e16c-11d2-bb90-00c04f8ee6c0"))
ISpObjectToken : ISpDataKey
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetId (
        LPWSTR pszCategoryId,
        LPWSTR pszTokenId,
        long fCreateIfNotExist ) = 0;
      virtual HRESULT __stdcall GetId (
        /*[out]*/ LPWSTR * ppszCoMemTokenId ) = 0;
      virtual HRESULT __stdcall GetCategory (
        struct ISpObjectTokenCategory * * ppTokenCategory ) = 0;
      virtual HRESULT __stdcall CreateInstance (
        /*[in]*/ IUnknown * pUnkOuter,
        /*[in]*/ unsigned long dwClsContext,
        /*[in]*/ GUID * riid,
        /*[out]*/ void * * ppvObject ) = 0;
      virtual HRESULT __stdcall GetStorageFileName (
        /*[in]*/ GUID * clsidCaller,
        /*[in]*/ LPWSTR pszValueName,
        /*[in]*/ LPWSTR pszFileNameSpecifier,
        /*[in]*/ unsigned long nFolder,
        /*[out]*/ LPWSTR * ppszFilePath ) = 0;
      virtual HRESULT __stdcall RemoveStorageFileName (
        /*[in]*/ GUID * clsidCaller,
        /*[in]*/ LPWSTR pszKeyName,
        /*[in]*/ long fDeleteFile ) = 0;
      virtual HRESULT __stdcall Remove (
        GUID * pclsidCaller ) = 0;
      virtual HRESULT __stdcall IsUISupported (
        /*[in]*/ LPWSTR pszTypeOfUI,
        /*[in]*/ void * pvExtraData,
        /*[in]*/ unsigned long cbExtraData,
        /*[in]*/ IUnknown * punkObject,
        /*[out]*/ long * pfSupported ) = 0;
      virtual HRESULT __stdcall DisplayUI (
        /*[in]*/ wireHWND hWndParent,
        /*[in]*/ LPWSTR pszTitle,
        /*[in]*/ LPWSTR pszTypeOfUI,
        /*[in]*/ void * pvExtraData,
        /*[in]*/ unsigned long cbExtraData,
        /*[in]*/ IUnknown * punkObject ) = 0;
      virtual HRESULT __stdcall MatchesAttributes (
        /*[in]*/ LPWSTR pszAttributes,
        /*[out]*/ long * pfMatches ) = 0;
};

struct __declspec(uuid("06b64f9e-7fda-11d2-b4f2-00c04f797396"))
IEnumSpObjectTokens : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Next (
        /*[in]*/ unsigned long celt,
        /*[out]*/ struct ISpObjectToken * * pelt,
        /*[out]*/ unsigned long * pceltFetched ) = 0;
      virtual HRESULT __stdcall Skip (
        /*[in]*/ unsigned long celt ) = 0;
      virtual HRESULT __stdcall Reset ( ) = 0;
      virtual HRESULT __stdcall Clone (
        /*[out]*/ struct IEnumSpObjectTokens * * ppEnum ) = 0;
      virtual HRESULT __stdcall Item (
        /*[in]*/ unsigned long Index,
        /*[out]*/ struct ISpObjectToken * * ppToken ) = 0;
      virtual HRESULT __stdcall GetCount (
        /*[out]*/ unsigned long * pCount ) = 0;
};

struct __declspec(uuid("5b559f40-e952-11d2-bb91-00c04f8ee6c0"))
ISpObjectWithToken : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetObjectToken (
        struct ISpObjectToken * pToken ) = 0;
      virtual HRESULT __stdcall GetObjectToken (
        struct ISpObjectToken * * ppToken ) = 0;
};

struct __declspec(uuid("6c44df74-72b9-4992-a1ec-ef996e0422d4"))
ISpVoice : ISpEventSource
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetOutput (
        /*[in]*/ IUnknown * pUnkOutput,
        /*[in]*/ long fAllowFormatChanges ) = 0;
      virtual HRESULT __stdcall GetOutputObjectToken (
        /*[out]*/ struct ISpObjectToken * * ppObjectToken ) = 0;
      virtual HRESULT __stdcall GetOutputStream (
        /*[out]*/ struct ISpStreamFormat * * ppStream ) = 0;
      virtual HRESULT __stdcall Pause ( ) = 0;
      virtual HRESULT __stdcall Resume ( ) = 0;
      virtual HRESULT __stdcall SetVoice (
        /*[in]*/ struct ISpObjectToken * pToken ) = 0;
      virtual HRESULT __stdcall GetVoice (
        /*[out]*/ struct ISpObjectToken * * ppToken ) = 0;
      virtual HRESULT __stdcall Speak (
        /*[in]*/ LPWSTR pwcs,
        /*[in]*/ unsigned long dwFlags,
        /*[out]*/ unsigned long * pulStreamNumber ) = 0;
      virtual HRESULT __stdcall SpeakStream (
        /*[in]*/ struct IStream * pStream,
        /*[in]*/ unsigned long dwFlags,
        /*[out]*/ unsigned long * pulStreamNumber ) = 0;
      virtual HRESULT __stdcall GetStatus (
        /*[out]*/ struct SPVOICESTATUS * pStatus,
        /*[out]*/ LPWSTR * ppszLastBookmark ) = 0;
      virtual HRESULT __stdcall Skip (
        /*[in]*/ LPWSTR pItemType,
        /*[in]*/ long lNumItems,
        /*[out]*/ unsigned long * pulNumSkipped ) = 0;
      virtual HRESULT __stdcall SetPriority (
        /*[in]*/ enum SPVPRIORITY ePriority ) = 0;
      virtual HRESULT __stdcall GetPriority (
        /*[out]*/ enum SPVPRIORITY * pePriority ) = 0;
      virtual HRESULT __stdcall SetAlertBoundary (
        /*[in]*/ enum SPEVENTENUM eBoundary ) = 0;
      virtual HRESULT __stdcall GetAlertBoundary (
        /*[out]*/ enum SPEVENTENUM * peBoundary ) = 0;
      virtual HRESULT __stdcall SetRate (
        /*[in]*/ long RateAdjust ) = 0;
      virtual HRESULT __stdcall GetRate (
        /*[out]*/ long * pRateAdjust ) = 0;
      virtual HRESULT __stdcall SetVolume (
        /*[in]*/ unsigned short usVolume ) = 0;
      virtual HRESULT __stdcall GetVolume (
        /*[out]*/ unsigned short * pusVolume ) = 0;
      virtual HRESULT __stdcall WaitUntilDone (
        /*[in]*/ unsigned long msTimeout ) = 0;
      virtual HRESULT __stdcall SetSyncSpeakTimeout (
        /*[in]*/ unsigned long msTimeout ) = 0;
      virtual HRESULT __stdcall GetSyncSpeakTimeout (
        /*[out]*/ unsigned long * pmsTimeout ) = 0;
      virtual void * __stdcall SpeakCompleteEvent ( ) = 0;
      virtual HRESULT __stdcall IsUISupported (
        /*[in]*/ LPWSTR pszTypeOfUI,
        /*[in]*/ void * pvExtraData,
        /*[in]*/ unsigned long cbExtraData,
        /*[out]*/ long * pfSupported ) = 0;
      virtual HRESULT __stdcall DisplayUI (
        /*[in]*/ wireHWND hWndParent,
        /*[in]*/ LPWSTR pszTitle,
        /*[in]*/ LPWSTR pszTypeOfUI,
        /*[in]*/ void * pvExtraData,
        /*[in]*/ unsigned long cbExtraData ) = 0;
};

struct __declspec(uuid("8445c581-0cac-4a38-abfe-9b2ce2826455"))
ISpPhoneConverter : ISpObjectWithToken
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall PhoneToId (
        /*[in]*/ LPWSTR pszPhone,
        /*[out]*/ unsigned short * pId ) = 0;
      virtual HRESULT __stdcall IdToPhone (
        /*[in]*/ LPWSTR pId,
        /*[out]*/ unsigned short * pszPhone ) = 0;
};

struct __declspec(uuid("f740a62f-7c15-489e-8234-940a33d9272d"))
ISpRecoContext : ISpEventSource
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetRecognizer (
        /*[out]*/ struct ISpRecognizer * * ppRecognizer ) = 0;
      virtual HRESULT __stdcall CreateGrammar (
        /*[in]*/ unsigned __int64 ullGrammarID,
        /*[out]*/ struct ISpRecoGrammar * * ppGrammar ) = 0;
      virtual HRESULT __stdcall GetStatus (
        /*[out]*/ struct SPRECOCONTEXTSTATUS * pStatus ) = 0;
      virtual HRESULT __stdcall GetMaxAlternates (
        /*[in]*/ unsigned long * pcAlternates ) = 0;
      virtual HRESULT __stdcall SetMaxAlternates (
        /*[in]*/ unsigned long cAlternates ) = 0;
      virtual HRESULT __stdcall SetAudioOptions (
        /*[in]*/ enum SPAUDIOOPTIONS Options,
        /*[in]*/ GUID * pAudioFormatId,
        /*[in]*/ struct WaveFormatEx * pWaveFormatEx ) = 0;
      virtual HRESULT __stdcall GetAudioOptions (
        /*[in]*/ enum SPAUDIOOPTIONS * pOptions,
        /*[out]*/ GUID * pAudioFormatId,
        /*[out]*/ struct WaveFormatEx * * ppCoMemWFEX ) = 0;
      virtual HRESULT __stdcall DeserializeResult (
        /*[in]*/ struct SPSERIALIZEDRESULT * pSerializedResult,
        /*[out]*/ struct ISpRecoResult * * ppResult ) = 0;
      virtual HRESULT __stdcall Bookmark (
        /*[in]*/ enum SPBOOKMARKOPTIONS Options,
        /*[in]*/ unsigned __int64 ullStreamPosition,
        /*[in]*/ LONG_PTR lparamEvent ) = 0;
      virtual HRESULT __stdcall SetAdaptationData (
        /*[in]*/ LPWSTR pAdaptationData,
        /*[in]*/ unsigned long cch ) = 0;
      virtual HRESULT __stdcall Pause (
        unsigned long dwReserved ) = 0;
      virtual HRESULT __stdcall Resume (
        unsigned long dwReserved ) = 0;
      virtual HRESULT __stdcall SetVoice (
        /*[in]*/ struct ISpVoice * pVoice,
        /*[in]*/ long fAllowFormatChanges ) = 0;
      virtual HRESULT __stdcall GetVoice (
        /*[out]*/ struct ISpVoice * * ppVoice ) = 0;
      virtual HRESULT __stdcall SetVoicePurgeEvent (
        /*[in]*/ unsigned __int64 ullEventInterest ) = 0;
      virtual HRESULT __stdcall GetVoicePurgeEvent (
        /*[out]*/ unsigned __int64 * pullEventInterest ) = 0;
      virtual HRESULT __stdcall SetContextState (
        /*[in]*/ enum SPCONTEXTSTATE eContextState ) = 0;
      virtual HRESULT __stdcall GetContextState (
        /*[out]*/ enum SPCONTEXTSTATE * peContextState ) = 0;
};

struct __declspec(uuid("c2b5f241-daa0-4507-9e16-5a1eaa2b7a5c"))
ISpRecognizer : ISpProperties
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetRecognizer (
        /*[in]*/ struct ISpObjectToken * pRecognizer ) = 0;
      virtual HRESULT __stdcall GetRecognizer (
        /*[out]*/ struct ISpObjectToken * * ppRecognizer ) = 0;
      virtual HRESULT __stdcall SetInput (
        /*[in]*/ IUnknown * pUnkInput,
        /*[in]*/ long fAllowFormatChanges ) = 0;
      virtual HRESULT __stdcall GetInputObjectToken (
        /*[out]*/ struct ISpObjectToken * * ppToken ) = 0;
      virtual HRESULT __stdcall GetInputStream (
        /*[out]*/ struct ISpStreamFormat * * ppStream ) = 0;
      virtual HRESULT __stdcall CreateRecoContext (
        /*[out]*/ struct ISpRecoContext * * ppNewCtxt ) = 0;
      virtual HRESULT __stdcall GetRecoProfile (
        /*[out]*/ struct ISpObjectToken * * ppToken ) = 0;
      virtual HRESULT __stdcall SetRecoProfile (
        /*[in]*/ struct ISpObjectToken * pToken ) = 0;
      virtual HRESULT __stdcall IsSharedInstance ( ) = 0;
      virtual HRESULT __stdcall GetRecoState (
        /*[out]*/ enum SPRECOSTATE * pState ) = 0;
      virtual HRESULT __stdcall SetRecoState (
        /*[in]*/ enum SPRECOSTATE NewState ) = 0;
      virtual HRESULT __stdcall GetStatus (
        /*[out]*/ struct SPRECOGNIZERSTATUS * pStatus ) = 0;
      virtual HRESULT __stdcall GetFormat (
        /*[in]*/ SPSTREAMFORMATTYPE WaveFormatType,
        /*[out]*/ GUID * pFormatId,
        /*[out]*/ struct WaveFormatEx * * ppCoMemWFEX ) = 0;
      virtual HRESULT __stdcall IsUISupported (
        /*[in]*/ LPWSTR pszTypeOfUI,
        /*[in]*/ void * pvExtraData,
        /*[in]*/ unsigned long cbExtraData,
        /*[out]*/ long * pfSupported ) = 0;
      virtual HRESULT __stdcall DisplayUI (
        /*[in]*/ wireHWND hWndParent,
        /*[in]*/ LPWSTR pszTitle,
        /*[in]*/ LPWSTR pszTypeOfUI,
        /*[in]*/ void * pvExtraData,
        /*[in]*/ unsigned long cbExtraData ) = 0;
      virtual HRESULT __stdcall EmulateRecognition (
        /*[in]*/ struct ISpPhrase * pPhrase ) = 0;
};

struct __declspec(uuid("2177db29-7f45-47d0-8554-067e91c80502"))
ISpRecoGrammar : ISpGrammarBuilder
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetGrammarId (
        /*[out]*/ unsigned __int64 * pullGrammarId ) = 0;
      virtual HRESULT __stdcall GetRecoContext (
        /*[out]*/ struct ISpRecoContext * * ppRecoCtxt ) = 0;
      virtual HRESULT __stdcall LoadCmdFromFile (
        /*[in]*/ LPWSTR pszFileName,
        /*[in]*/ enum SPLOADOPTIONS Options ) = 0;
      virtual HRESULT __stdcall LoadCmdFromObject (
        /*[in]*/ GUID * rcid,
        /*[in]*/ LPWSTR pszGrammarName,
        /*[in]*/ enum SPLOADOPTIONS Options ) = 0;
      virtual HRESULT __stdcall LoadCmdFromResource (
        /*[in]*/ void * hModule,
        /*[in]*/ LPWSTR pszResourceName,
        /*[in]*/ LPWSTR pszResourceType,
        /*[in]*/ unsigned short wLanguage,
        /*[in]*/ enum SPLOADOPTIONS Options ) = 0;
      virtual HRESULT __stdcall LoadCmdFromMemory (
        /*[in]*/ struct SPBINARYGRAMMAR * pGrammar,
        /*[in]*/ enum SPLOADOPTIONS Options ) = 0;
      virtual HRESULT __stdcall LoadCmdFromProprietaryGrammar (
        /*[in]*/ GUID * rguidParam,
        /*[in]*/ LPWSTR pszStringParam,
        /*[in]*/ void * pvDataPrarm,
        /*[in]*/ unsigned long cbDataSize,
        /*[in]*/ enum SPLOADOPTIONS Options ) = 0;
      virtual HRESULT __stdcall SetRuleState (
        /*[in]*/ LPWSTR pszName,
        void * pReserved,
        /*[in]*/ enum SPRULESTATE NewState ) = 0;
      virtual HRESULT __stdcall SetRuleIdState (
        /*[in]*/ unsigned long ulRuleId,
        /*[in]*/ enum SPRULESTATE NewState ) = 0;
      virtual HRESULT __stdcall LoadDictation (
        /*[in]*/ LPWSTR pszTopicName,
        /*[in]*/ enum SPLOADOPTIONS Options ) = 0;
      virtual HRESULT __stdcall UnloadDictation ( ) = 0;
      virtual HRESULT __stdcall SetDictationState (
        /*[in]*/ enum SPRULESTATE NewState ) = 0;
      virtual HRESULT __stdcall SetWordSequenceData (
        /*[in]*/ unsigned short * pText,
        /*[in]*/ unsigned long cchText,
        /*[in]*/ SPTEXTSELECTIONINFO * pInfo ) = 0;
      virtual HRESULT __stdcall SetTextSelection (
        /*[in]*/ SPTEXTSELECTIONINFO * pInfo ) = 0;
      virtual HRESULT __stdcall IsPronounceable (
        /*[in]*/ LPWSTR pszWord,
        /*[out]*/ enum SPWORDPRONOUNCEABLE * pWordPronounceable ) = 0;
      virtual HRESULT __stdcall SetGrammarState (
        /*[in]*/ enum SPGRAMMARSTATE eGrammarState ) = 0;
      virtual HRESULT __stdcall SaveCmd (
        /*[in]*/ struct IStream * pStream,
        /*[out]*/ LPWSTR * ppszCoMemErrorText ) = 0;
      virtual HRESULT __stdcall GetGrammarState (
        /*[out]*/ enum SPGRAMMARSTATE * peGrammarState ) = 0;
};

struct __declspec(uuid("20b053be-e235-43cd-9a2a-8d17a48b7842"))
ISpRecoResult : ISpPhrase
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetResultTimes (
        /*[out]*/ struct SPRECORESULTTIMES * pTimes ) = 0;
      virtual HRESULT __stdcall GetAlternates (
        /*[in]*/ unsigned long ulStartElement,
        /*[in]*/ unsigned long cElements,
        /*[in]*/ unsigned long ulRequestCount,
        /*[out]*/ struct ISpPhraseAlt * * ppPhrases,
        /*[out]*/ unsigned long * pcPhrasesReturned ) = 0;
      virtual HRESULT __stdcall GetAudio (
        /*[in]*/ unsigned long ulStartElement,
        /*[in]*/ unsigned long cElements,
        /*[out]*/ struct ISpStreamFormat * * ppStream ) = 0;
      virtual HRESULT __stdcall SpeakAudio (
        /*[in]*/ unsigned long ulStartElement,
        /*[in]*/ unsigned long cElements,
        /*[in]*/ unsigned long dwFlags,
        /*[out]*/ unsigned long * pulStreamNumber ) = 0;
      virtual HRESULT __stdcall Serialize (
        /*[out]*/ struct SPSERIALIZEDRESULT * * ppCoMemSerializedResult ) = 0;
      virtual HRESULT __stdcall ScaleAudio (
        /*[in]*/ GUID * pAudioFormatId,
        /*[in]*/ struct WaveFormatEx * pWaveFormatEx ) = 0;
      virtual HRESULT __stdcall GetRecoContext (
        /*[out]*/ struct ISpRecoContext * * ppRecoContext ) = 0;
};

struct __declspec(uuid("ae39362b-45a8-4074-9b9e-ccf49aa2d0b6"))
ISpXMLRecoResult : ISpRecoResult
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetXMLResult (
        /*[out]*/ LPWSTR * ppszCoMemXMLResult,
        /*[in]*/ enum SPXMLRESULTOPTIONS Options ) = 0;
      virtual HRESULT __stdcall GetXMLErrorInfo (
        struct SPSEMANTICERRORINFO * pSemanticErrorInfo ) = 0;
};

} // namespace SpeechLib

#pragma pack(pop)
