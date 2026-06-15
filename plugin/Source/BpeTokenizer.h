#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>

class BpeTokenizer
{
public:
    BpeTokenizer();
    ~BpeTokenizer();

    bool load(const juce::File& vocabFile, const juce::File& mergesFile);
    bool isLoaded() const { return loaded; }

    std::vector<std::string> tokenize(const std::string& text);
    int64_t convertTokenToId(const std::string& token);

    void encode(const std::string& text, size_t maxLength, 
                std::vector<int64_t>& inputIds, 
                std::vector<int64_t>& maskIds);

private:
    typedef std::pair<std::wstring, std::wstring> wbigram_pair;

    struct HashPair 
    {
        template <class T1, class T2>
        size_t operator()(const std::pair<T1, T2>& p) const
        {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };

    bool loaded = false;
    
    std::string bosToken = "<s>";
    std::string eosToken = "</s>";
    std::string padToken = "<pad>";
    std::string unkToken = "<unk>";

    std::unordered_map<std::string, int64_t> vocab;
    std::unordered_map<wbigram_pair, uint32_t, HashPair> bpeRanks;
    std::vector<wchar_t> bytesEncoder;
    std::unordered_map<std::wstring, std::vector<std::string>> cache;

    std::wstring patExpr;
    std::unique_ptr<std::wregex> patRegex;

    void initBytesToUnicode();
    std::vector<std::string> bpe(const std::wstring& token);
    std::vector<std::wstring> getPairs(const std::vector<std::wstring>& word);
};
