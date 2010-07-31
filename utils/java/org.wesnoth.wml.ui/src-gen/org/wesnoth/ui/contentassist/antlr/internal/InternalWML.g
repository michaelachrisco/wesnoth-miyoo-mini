/*
* generated by Xtext
*/
grammar InternalWML;

options {
	superClass=AbstractInternalContentAssistParser;
	
}

@lexer::header {
package org.wesnoth.ui.contentassist.antlr.internal;

// Hack: Use our own Lexer superclass by means of import. 
// Currently there is no other way to specify the superclass for the lexer.
import org.eclipse.xtext.ui.editor.contentassist.antlr.internal.Lexer;
}

@parser::header {
package org.wesnoth.ui.contentassist.antlr.internal; 

import java.io.InputStream;
import org.eclipse.xtext.*;
import org.eclipse.xtext.parser.*;
import org.eclipse.xtext.parser.impl.*;
import org.eclipse.xtext.parsetree.*;
import org.eclipse.emf.ecore.util.EcoreUtil;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.xtext.parser.antlr.XtextTokenStream;
import org.eclipse.xtext.parser.antlr.XtextTokenStream.HiddenTokens;
import org.eclipse.xtext.ui.editor.contentassist.antlr.internal.AbstractInternalContentAssistParser;
import org.eclipse.xtext.ui.editor.contentassist.antlr.internal.DFA;
import org.wesnoth.services.WMLGrammarAccess;

}

@parser::members {
 
 	private WMLGrammarAccess grammarAccess;
 	
    public void setGrammarAccess(WMLGrammarAccess grammarAccess) {
    	this.grammarAccess = grammarAccess;
    }
    
    @Override
    protected Grammar getGrammar() {
    	return grammarAccess.getGrammar();
    }
    
    @Override
    protected String getValueForTokenName(String tokenName) {
    	return tokenName;
    }

}




// Entry rule entryRuleWMLRoot
entryRuleWMLRoot 
:
{ before(grammarAccess.getWMLRootRule()); }
	 ruleWMLRoot
{ after(grammarAccess.getWMLRootRule()); } 
	 EOF 
;

// Rule WMLRoot
ruleWMLRoot
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLRootAccess().getAlternatives()); }
(rule__WMLRoot__Alternatives)*
{ after(grammarAccess.getWMLRootAccess().getAlternatives()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLTag
entryRuleWMLTag 
:
{ before(grammarAccess.getWMLTagRule()); }
	 ruleWMLTag
{ after(grammarAccess.getWMLTagRule()); } 
	 EOF 
;

// Rule WMLTag
ruleWMLTag
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLTagAccess().getGroup()); }
(rule__WMLTag__Group__0)
{ after(grammarAccess.getWMLTagAccess().getGroup()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLKey
entryRuleWMLKey 
@init {
	HiddenTokens myHiddenTokenState = ((XtextTokenStream)input).setHiddenTokens("RULE_WS");
}
:
{ before(grammarAccess.getWMLKeyRule()); }
	 ruleWMLKey
{ after(grammarAccess.getWMLKeyRule()); } 
	 EOF 
;
finally {
	myHiddenTokenState.restore();
}

// Rule WMLKey
ruleWMLKey
    @init {
		HiddenTokens myHiddenTokenState = ((XtextTokenStream)input).setHiddenTokens("RULE_WS");
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLKeyAccess().getGroup()); }
(rule__WMLKey__Group__0)
{ after(grammarAccess.getWMLKeyAccess().getGroup()); }
)

;
finally {
	restoreStackSize(stackSize);
	myHiddenTokenState.restore();
}



// Entry rule entryRuleWMLKeyValue
entryRuleWMLKeyValue 
:
{ before(grammarAccess.getWMLKeyValueRule()); }
	 ruleWMLKeyValue
{ after(grammarAccess.getWMLKeyValueRule()); } 
	 EOF 
;

// Rule WMLKeyValue
ruleWMLKeyValue
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLKeyValueAccess().getAlternatives()); }
(rule__WMLKeyValue__Alternatives)
{ after(grammarAccess.getWMLKeyValueAccess().getAlternatives()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLMacroCall
entryRuleWMLMacroCall 
:
{ before(grammarAccess.getWMLMacroCallRule()); }
	 ruleWMLMacroCall
{ after(grammarAccess.getWMLMacroCallRule()); } 
	 EOF 
;

// Rule WMLMacroCall
ruleWMLMacroCall
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLMacroCallAccess().getNameAssignment()); }
(rule__WMLMacroCall__NameAssignment)
{ after(grammarAccess.getWMLMacroCallAccess().getNameAssignment()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLLuaCode
entryRuleWMLLuaCode 
:
{ before(grammarAccess.getWMLLuaCodeRule()); }
	 ruleWMLLuaCode
{ after(grammarAccess.getWMLLuaCodeRule()); } 
	 EOF 
;

// Rule WMLLuaCode
ruleWMLLuaCode
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLLuaCodeAccess().getValueAssignment()); }
(rule__WMLLuaCode__ValueAssignment)
{ after(grammarAccess.getWMLLuaCodeAccess().getValueAssignment()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLArrayCall
entryRuleWMLArrayCall 
:
{ before(grammarAccess.getWMLArrayCallRule()); }
	 ruleWMLArrayCall
{ after(grammarAccess.getWMLArrayCallRule()); } 
	 EOF 
;

// Rule WMLArrayCall
ruleWMLArrayCall
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLArrayCallAccess().getGroup()); }
(rule__WMLArrayCall__Group__0)
{ after(grammarAccess.getWMLArrayCallAccess().getGroup()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLMacroDefine
entryRuleWMLMacroDefine 
:
{ before(grammarAccess.getWMLMacroDefineRule()); }
	 ruleWMLMacroDefine
{ after(grammarAccess.getWMLMacroDefineRule()); } 
	 EOF 
;

// Rule WMLMacroDefine
ruleWMLMacroDefine
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLMacroDefineAccess().getNameAssignment()); }
(rule__WMLMacroDefine__NameAssignment)
{ after(grammarAccess.getWMLMacroDefineAccess().getNameAssignment()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLTextdomain
entryRuleWMLTextdomain 
:
{ before(grammarAccess.getWMLTextdomainRule()); }
	 ruleWMLTextdomain
{ after(grammarAccess.getWMLTextdomainRule()); } 
	 EOF 
;

// Rule WMLTextdomain
ruleWMLTextdomain
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLTextdomainAccess().getNameAssignment()); }
(rule__WMLTextdomain__NameAssignment)
{ after(grammarAccess.getWMLTextdomainAccess().getNameAssignment()); }
)

;
finally {
	restoreStackSize(stackSize);
}



// Entry rule entryRuleWMLValue
entryRuleWMLValue 
:
{ before(grammarAccess.getWMLValueRule()); }
	 ruleWMLValue
{ after(grammarAccess.getWMLValueRule()); } 
	 EOF 
;

// Rule WMLValue
ruleWMLValue
    @init {
		int stackSize = keepStackSize();
    }
	:
(
{ before(grammarAccess.getWMLValueAccess().getValueAssignment()); }
(rule__WMLValue__ValueAssignment)
{ after(grammarAccess.getWMLValueAccess().getValueAssignment()); }
)

;
finally {
	restoreStackSize(stackSize);
}




rule__WMLRoot__Alternatives
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLRootAccess().getTagsAssignment_0()); }
(rule__WMLRoot__TagsAssignment_0)
{ after(grammarAccess.getWMLRootAccess().getTagsAssignment_0()); }
)

    |(
{ before(grammarAccess.getWMLRootAccess().getMacroCallsAssignment_1()); }
(rule__WMLRoot__MacroCallsAssignment_1)
{ after(grammarAccess.getWMLRootAccess().getMacroCallsAssignment_1()); }
)

    |(
{ before(grammarAccess.getWMLRootAccess().getMacroDefinesAssignment_2()); }
(rule__WMLRoot__MacroDefinesAssignment_2)
{ after(grammarAccess.getWMLRootAccess().getMacroDefinesAssignment_2()); }
)

    |(
{ before(grammarAccess.getWMLRootAccess().getTextdomainsAssignment_3()); }
(rule__WMLRoot__TextdomainsAssignment_3)
{ after(grammarAccess.getWMLRootAccess().getTextdomainsAssignment_3()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Alternatives_4
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getTagsAssignment_4_0()); }
(rule__WMLTag__TagsAssignment_4_0)
{ after(grammarAccess.getWMLTagAccess().getTagsAssignment_4_0()); }
)

    |(
{ before(grammarAccess.getWMLTagAccess().getKeysAssignment_4_1()); }
(rule__WMLTag__KeysAssignment_4_1)
{ after(grammarAccess.getWMLTagAccess().getKeysAssignment_4_1()); }
)

    |(
{ before(grammarAccess.getWMLTagAccess().getMacroCallsAssignment_4_2()); }
(rule__WMLTag__MacroCallsAssignment_4_2)
{ after(grammarAccess.getWMLTagAccess().getMacroCallsAssignment_4_2()); }
)

    |(
{ before(grammarAccess.getWMLTagAccess().getMacroDefinesAssignment_4_3()); }
(rule__WMLTag__MacroDefinesAssignment_4_3)
{ after(grammarAccess.getWMLTagAccess().getMacroDefinesAssignment_4_3()); }
)

    |(
{ before(grammarAccess.getWMLTagAccess().getTextdomainsAssignment_4_4()); }
(rule__WMLTag__TextdomainsAssignment_4_4)
{ after(grammarAccess.getWMLTagAccess().getTextdomainsAssignment_4_4()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__Alternatives_3
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyAccess().getEOLTerminalRuleCall_3_0()); }
	RULE_EOL
{ after(grammarAccess.getWMLKeyAccess().getEOLTerminalRuleCall_3_0()); }
)

    |(
{ before(grammarAccess.getWMLKeyAccess().getSL_COMMENTTerminalRuleCall_3_1()); }
	RULE_SL_COMMENT
{ after(grammarAccess.getWMLKeyAccess().getSL_COMMENTTerminalRuleCall_3_1()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKeyValue__Alternatives
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyValueAccess().getWMLValueParserRuleCall_0()); }
	ruleWMLValue
{ after(grammarAccess.getWMLKeyValueAccess().getWMLValueParserRuleCall_0()); }
)

    |(
{ before(grammarAccess.getWMLKeyValueAccess().getWMLMacroCallParserRuleCall_1()); }
	ruleWMLMacroCall
{ after(grammarAccess.getWMLKeyValueAccess().getWMLMacroCallParserRuleCall_1()); }
)

    |(
{ before(grammarAccess.getWMLKeyValueAccess().getWMLLuaCodeParserRuleCall_2()); }
	ruleWMLLuaCode
{ after(grammarAccess.getWMLKeyValueAccess().getWMLLuaCodeParserRuleCall_2()); }
)

    |(
{ before(grammarAccess.getWMLKeyValueAccess().getWMLArrayCallParserRuleCall_3()); }
	ruleWMLArrayCall
{ after(grammarAccess.getWMLKeyValueAccess().getWMLArrayCallParserRuleCall_3()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLValue__ValueAlternatives_0
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLValueAccess().getValueIDTerminalRuleCall_0_0()); }
	RULE_ID
{ after(grammarAccess.getWMLValueAccess().getValueIDTerminalRuleCall_0_0()); }
)

    |(
{ before(grammarAccess.getWMLValueAccess().getValueSTRINGTerminalRuleCall_0_1()); }
	RULE_STRING
{ after(grammarAccess.getWMLValueAccess().getValueSTRINGTerminalRuleCall_0_1()); }
)

    |(
{ before(grammarAccess.getWMLValueAccess().getValueANY_OTHERTerminalRuleCall_0_2()); }
	RULE_ANY_OTHER
{ after(grammarAccess.getWMLValueAccess().getValueANY_OTHERTerminalRuleCall_0_2()); }
)

    |(
{ before(grammarAccess.getWMLValueAccess().getValuePlusSignKeyword_0_3()); }

	'+' 

{ after(grammarAccess.getWMLValueAccess().getValuePlusSignKeyword_0_3()); }
)

;
finally {
	restoreStackSize(stackSize);
}



rule__WMLTag__Group__0
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__0__Impl
	rule__WMLTag__Group__1
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__0__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getLeftSquareBracketKeyword_0()); }

	'[' 

{ after(grammarAccess.getWMLTagAccess().getLeftSquareBracketKeyword_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__1
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__1__Impl
	rule__WMLTag__Group__2
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__1__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getPlusAssignment_1()); }
(rule__WMLTag__PlusAssignment_1)?
{ after(grammarAccess.getWMLTagAccess().getPlusAssignment_1()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__2
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__2__Impl
	rule__WMLTag__Group__3
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__2__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getNameAssignment_2()); }
(rule__WMLTag__NameAssignment_2)
{ after(grammarAccess.getWMLTagAccess().getNameAssignment_2()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__3
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__3__Impl
	rule__WMLTag__Group__4
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__3__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getRightSquareBracketKeyword_3()); }

	']' 

{ after(grammarAccess.getWMLTagAccess().getRightSquareBracketKeyword_3()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__4
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__4__Impl
	rule__WMLTag__Group__5
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__4__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getAlternatives_4()); }
(rule__WMLTag__Alternatives_4)*
{ after(grammarAccess.getWMLTagAccess().getAlternatives_4()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__5
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__5__Impl
	rule__WMLTag__Group__6
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__5__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getLeftSquareBracketSolidusKeyword_5()); }

	'[/' 

{ after(grammarAccess.getWMLTagAccess().getLeftSquareBracketSolidusKeyword_5()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__6
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__6__Impl
	rule__WMLTag__Group__7
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__6__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getEndNameAssignment_6()); }
(rule__WMLTag__EndNameAssignment_6)
{ after(grammarAccess.getWMLTagAccess().getEndNameAssignment_6()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLTag__Group__7
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLTag__Group__7__Impl
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__Group__7__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getRightSquareBracketKeyword_7()); }

	']' 

{ after(grammarAccess.getWMLTagAccess().getRightSquareBracketKeyword_7()); }
)

;
finally {
	restoreStackSize(stackSize);
}


















rule__WMLKey__Group__0
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLKey__Group__0__Impl
	rule__WMLKey__Group__1
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__Group__0__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyAccess().getNameAssignment_0()); }
(rule__WMLKey__NameAssignment_0)
{ after(grammarAccess.getWMLKeyAccess().getNameAssignment_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLKey__Group__1
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLKey__Group__1__Impl
	rule__WMLKey__Group__2
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__Group__1__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyAccess().getEqualsSignKeyword_1()); }

	'=' 

{ after(grammarAccess.getWMLKeyAccess().getEqualsSignKeyword_1()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLKey__Group__2
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLKey__Group__2__Impl
	rule__WMLKey__Group__3
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__Group__2__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
(
{ before(grammarAccess.getWMLKeyAccess().getValueAssignment_2()); }
(rule__WMLKey__ValueAssignment_2)
{ after(grammarAccess.getWMLKeyAccess().getValueAssignment_2()); }
)
(
{ before(grammarAccess.getWMLKeyAccess().getValueAssignment_2()); }
(rule__WMLKey__ValueAssignment_2)*
{ after(grammarAccess.getWMLKeyAccess().getValueAssignment_2()); }
)
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLKey__Group__3
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLKey__Group__3__Impl
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__Group__3__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyAccess().getAlternatives_3()); }
(rule__WMLKey__Alternatives_3)
{ after(grammarAccess.getWMLKeyAccess().getAlternatives_3()); }
)

;
finally {
	restoreStackSize(stackSize);
}










rule__WMLArrayCall__Group__0
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLArrayCall__Group__0__Impl
	rule__WMLArrayCall__Group__1
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLArrayCall__Group__0__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLArrayCallAccess().getLeftSquareBracketKeyword_0()); }

	'[' 

{ after(grammarAccess.getWMLArrayCallAccess().getLeftSquareBracketKeyword_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLArrayCall__Group__1
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLArrayCall__Group__1__Impl
	rule__WMLArrayCall__Group__2
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLArrayCall__Group__1__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
(
{ before(grammarAccess.getWMLArrayCallAccess().getValueAssignment_1()); }
(rule__WMLArrayCall__ValueAssignment_1)
{ after(grammarAccess.getWMLArrayCallAccess().getValueAssignment_1()); }
)
(
{ before(grammarAccess.getWMLArrayCallAccess().getValueAssignment_1()); }
(rule__WMLArrayCall__ValueAssignment_1)*
{ after(grammarAccess.getWMLArrayCallAccess().getValueAssignment_1()); }
)
)

;
finally {
	restoreStackSize(stackSize);
}


rule__WMLArrayCall__Group__2
    @init {
		int stackSize = keepStackSize();
    }
:
	rule__WMLArrayCall__Group__2__Impl
;
finally {
	restoreStackSize(stackSize);
}

rule__WMLArrayCall__Group__2__Impl
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLArrayCallAccess().getRightSquareBracketKeyword_2()); }

	']' 

{ after(grammarAccess.getWMLArrayCallAccess().getRightSquareBracketKeyword_2()); }
)

;
finally {
	restoreStackSize(stackSize);
}









rule__WMLRoot__TagsAssignment_0
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLRootAccess().getTagsWMLTagParserRuleCall_0_0()); }
	ruleWMLTag{ after(grammarAccess.getWMLRootAccess().getTagsWMLTagParserRuleCall_0_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLRoot__MacroCallsAssignment_1
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLRootAccess().getMacroCallsWMLMacroCallParserRuleCall_1_0()); }
	ruleWMLMacroCall{ after(grammarAccess.getWMLRootAccess().getMacroCallsWMLMacroCallParserRuleCall_1_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLRoot__MacroDefinesAssignment_2
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLRootAccess().getMacroDefinesWMLMacroDefineParserRuleCall_2_0()); }
	ruleWMLMacroDefine{ after(grammarAccess.getWMLRootAccess().getMacroDefinesWMLMacroDefineParserRuleCall_2_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLRoot__TextdomainsAssignment_3
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLRootAccess().getTextdomainsWMLTextdomainParserRuleCall_3_0()); }
	ruleWMLTextdomain{ after(grammarAccess.getWMLRootAccess().getTextdomainsWMLTextdomainParserRuleCall_3_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__PlusAssignment_1
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getPlusPlusSignKeyword_1_0()); }
(
{ before(grammarAccess.getWMLTagAccess().getPlusPlusSignKeyword_1_0()); }

	'+' 

{ after(grammarAccess.getWMLTagAccess().getPlusPlusSignKeyword_1_0()); }
)

{ after(grammarAccess.getWMLTagAccess().getPlusPlusSignKeyword_1_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__NameAssignment_2
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getNameIDTerminalRuleCall_2_0()); }
	RULE_ID{ after(grammarAccess.getWMLTagAccess().getNameIDTerminalRuleCall_2_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__TagsAssignment_4_0
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getTagsWMLTagParserRuleCall_4_0_0()); }
	ruleWMLTag{ after(grammarAccess.getWMLTagAccess().getTagsWMLTagParserRuleCall_4_0_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__KeysAssignment_4_1
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getKeysWMLKeyParserRuleCall_4_1_0()); }
	ruleWMLKey{ after(grammarAccess.getWMLTagAccess().getKeysWMLKeyParserRuleCall_4_1_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__MacroCallsAssignment_4_2
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getMacroCallsWMLMacroCallParserRuleCall_4_2_0()); }
	ruleWMLMacroCall{ after(grammarAccess.getWMLTagAccess().getMacroCallsWMLMacroCallParserRuleCall_4_2_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__MacroDefinesAssignment_4_3
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getMacroDefinesWMLMacroDefineParserRuleCall_4_3_0()); }
	ruleWMLMacroDefine{ after(grammarAccess.getWMLTagAccess().getMacroDefinesWMLMacroDefineParserRuleCall_4_3_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__TextdomainsAssignment_4_4
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getTextdomainsWMLTextdomainParserRuleCall_4_4_0()); }
	ruleWMLTextdomain{ after(grammarAccess.getWMLTagAccess().getTextdomainsWMLTextdomainParserRuleCall_4_4_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTag__EndNameAssignment_6
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTagAccess().getEndNameIDTerminalRuleCall_6_0()); }
	RULE_ID{ after(grammarAccess.getWMLTagAccess().getEndNameIDTerminalRuleCall_6_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__NameAssignment_0
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyAccess().getNameIDTerminalRuleCall_0_0()); }
	RULE_ID{ after(grammarAccess.getWMLKeyAccess().getNameIDTerminalRuleCall_0_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLKey__ValueAssignment_2
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLKeyAccess().getValueWMLKeyValueParserRuleCall_2_0()); }
	ruleWMLKeyValue{ after(grammarAccess.getWMLKeyAccess().getValueWMLKeyValueParserRuleCall_2_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLMacroCall__NameAssignment
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLMacroCallAccess().getNameMACROTerminalRuleCall_0()); }
	RULE_MACRO{ after(grammarAccess.getWMLMacroCallAccess().getNameMACROTerminalRuleCall_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLLuaCode__ValueAssignment
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLLuaCodeAccess().getValueLUA_CODETerminalRuleCall_0()); }
	RULE_LUA_CODE{ after(grammarAccess.getWMLLuaCodeAccess().getValueLUA_CODETerminalRuleCall_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLArrayCall__ValueAssignment_1
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLArrayCallAccess().getValueWMLValueParserRuleCall_1_0()); }
	ruleWMLValue{ after(grammarAccess.getWMLArrayCallAccess().getValueWMLValueParserRuleCall_1_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLMacroDefine__NameAssignment
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLMacroDefineAccess().getNameDEFINETerminalRuleCall_0()); }
	RULE_DEFINE{ after(grammarAccess.getWMLMacroDefineAccess().getNameDEFINETerminalRuleCall_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLTextdomain__NameAssignment
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLTextdomainAccess().getNameTEXTDOMAINTerminalRuleCall_0()); }
	RULE_TEXTDOMAIN{ after(grammarAccess.getWMLTextdomainAccess().getNameTEXTDOMAINTerminalRuleCall_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}

rule__WMLValue__ValueAssignment
    @init {
		int stackSize = keepStackSize();
    }
:
(
{ before(grammarAccess.getWMLValueAccess().getValueAlternatives_0()); }
(rule__WMLValue__ValueAlternatives_0)
{ after(grammarAccess.getWMLValueAccess().getValueAlternatives_0()); }
)

;
finally {
	restoreStackSize(stackSize);
}


RULE_LUA_CODE : '<<' ( options {greedy=false;} : . )*'>>';

RULE_MACRO : '{' ( options {greedy=false;} : . )*'}';

RULE_DEFINE : '#define' ( options {greedy=false;} : . )*'#enddef';

RULE_TEXTDOMAIN : '#textdomain' ~(('\n'|'\r'))* ('\r'? '\n')?;

RULE_STRING : '"' ('\\' ('b'|'t'|'n'|'f'|'r'|'"'|'\''|'\\')|~(('\\'|'"')))* '"';

RULE_ID : ('a'..'z'|'A'..'Z'|'0'..'9'|'_'|',')+;

RULE_EOL : ('\r'|'\n')+;

RULE_WS : (' '|'\t')+;

RULE_ANY_OTHER : .;

RULE_SL_COMMENT : '#' ~(('\n'|'\r'))* ('\r'? '\n')?;


