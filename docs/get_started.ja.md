
# チュートリアル

ここではFaseの基本的な使い方を解説します.  

## Step 1. Faseクラスインスタンスの作成

Faseでは基本的に一つの`Fase`クラスのインスタンスを操作します.  
まずはその`Fase`インスタンスを作成します.  
`Fase`クラスはテンプレートクラスとなっており, 以下のように好きなExtensionクラスを
テンプレートパラメータとして渡し, 型を決定します.  

```cpp
fase::Fase<[Extension1], [Extension2], ...> app;
```

Extensionとして選べるものは`fase::PartsBase`クラスを継承したクラスです.  
あらかじめ用意されているものもありますが, 自分でクラスを作成しExtensionとして
使用するのもいいでしょう.  

## Step 2. 関数の登録

次にパイプラインの作成に使う関数の登録をします.  
登録の方法は, 二通りあります.  

### a. 動的に登録する方法

動的に関数を登録するには`FaseAddUnivFunction`マクロを使います.  

```cpp
FaseAddUnivFunction([関数名], ([型1, 型2, ...]), ("[引数名1]", "[引数名2]", ...),
                    [faseのインスタンス]);
FaseAddUnivFunction([関数名], ([型1, 型2, ...]), ("[引数名1]", "[引数名2]", ...),
                    [faseのインスタンス], "[関数の説明]");
FaseAddUnivFunction([関数名], ([型1, 型2, ...]), ("[引数名1]", "[引数名2]", ...)
                    [faseのインスタンス], "[関数の説明]",
                    {[デフォルト引数1], [デフォルト引数2], ...});
```

#### 例)

```cpp
void f(int i) {
    // ...
}
void g(const float& a, std::string b, int& dst) {
    // ...
}

// ...

fase::Fase< /* ... */ > app;

// 関数fの登録
FaseAddUnivFunction(app, f, (int), ("i"));
// 関数gの登録
FaseAddUnivFunction(app, g, (const float&, std::string, int&),
                    ("a", "b", "dst"));

// 関数gの登録 (説明付き)
FaseAddUnivFunction(app, g, (const float&, std::string, int&),
                    ("a", "b", "dst"), "do something like g.");

// 関数gの登録 (説明, デフォルト引数付き)
FaseAddUnivFunction(app, g, (const float&, std::string, int&),
                    ("a", "b", "dst"), "do something like g.",
                    {0, "default"s, 0});
```

### b. 静的に登録する方法

静的に関数を登録するには`FaseAutoAddingUnivFunction`マクロを使います.  
`FaseAutoAddingUnivFunction`マクロは関数の宣言時に使用します.  

```cpp
FaseAutoAddingUnivFunction([関数名],
[関数の宣言(および定義)]
)
```

#### 例)

```cpp
#include "fase.h"

FaseAutoAddingUnivFunction(f,
void f(int i) {
    // ...
}
)

FaseAutoAddingUnivFunction(g,
void g(const float& a, std::string b, int& dst) {
    // ...
}
)

// ...

fase::Fase< /* ... */ > app;
// インスタンスを作成した時点ですでにf とg は登録されている.
```

## 

## Step 3. 使う

これで`Fase`自体のセットアップは完了です.  
あとは各Extensionクラスの使い方に従います.  

[FaseParts](@ref FaseParts)グループの各クラスを見ましょう.  

ここでは例として, `ImGuiEditor`をあげると  
ImGuiを使ったレンダリングループの中で`ImGuiEditor::runEditting`を呼び出すことで,
ImGuiによるグラフィカルなパイプラインエディターを扱えます.  

具体的な例は`example/guieditor`に書いてありますので, 見てください.  
