from langchain_openai import ChatOpenAI
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.output_parsers import StrOutputParser


llm = ChatOpenAI(
    model="gpt-4o-mini",
    temperature=0.3
)

summarize_prompt = ChatPromptTemplate.from_template("""
You are an AI assistant that maintains a concise memory of a conversation.

Existing memory:
{memory}

New conversation:
{conversation}

Update the memory with the important information from the new conversation.
Keep it concise and preserve important facts, preferences, decisions, and context.
""")

summary_chain = (
    summarize_prompt
    | llm
    | StrOutputParser()
)

memory = ""

print("LangChain Conversation Memory")
print("Type 'exit' to quit.\n")

while True:
    user_input = input("You: ")

    if user_input.lower() == "exit":
        break

    memory = summary_chain.invoke({
        "memory": memory,
        "conversation": user_input
    })

    print("\nUpdated Memory:")
    print(memory)
    print("-" * 50) 