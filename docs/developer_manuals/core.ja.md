
# Core クラス

`Core` クラスは Fase の中核を担うクラスで, 一つのパイプラインを管理するクラスです.

中には 各ノードの情報, ノード間のリンクの情報, 各関数の情報 が保存されています.

Pimplパターンを採用しているます.

## デフォルトコンストラクタ

空のパイプラインを表すオブジェクトが生成されます.

## ムーブコンストラクタ, ムーブ代入演算子

自然なムーブコンストラクタを提供します.  
ちなみに現在, Pimplパターンを採用しているのでムーブのコストは,
`std::unique_ptr`のムーブコストのみでずいぶん軽いです.

## コピーコンストラクタ, コピー代入演算子

中身のディープコピーが行われパイプラインが複製されます.  
当然 `supposeInput`, `supposeOutput`, `setArgument` で紐づけた `Variable` は
新しくできた方のオブジェクトのものとは紐付けされていないので注意してください.

## `addUnivFunc`

```c++
bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                 std::vector<Variable>&& default_args)
```

ユニバーサル化された関数 `func` を `f_name` と言う名前で登録します.  
`default_args` に渡した `Variable` が ノードに関数が割り当てられた時に
コピーして引数として使われます.

渡した引数 `func` はコピーして内部に保存され,
各ノードに関数が設定される度にコピーされます.

関数の追加に成功した場合 `true` が返されます.

## `newNode`

```c++
bool newNode(const std::string& n_name);
```

新しく `n_name` という名前のノードを作成します.  
成功した場合 `true` が返されます.

## `renameNode`

```c++
bool renameNode(const std::string& old_n_name,
                const std::string& new_n_name);
```

すでにある `old_n_name` というノードを `new_n_name` と言う名前に変更します.  
成功した場合 `true` が返されます.

## `delNode`

```c++
bool delNode(const std::string& n_name);
```

すでにある `n_name` というノードを消去します.  
成功した場合 `true` が返されます.

## `setArgument`

```c++
bool setArgument(const std::string& n_name, std::size_t idx, Variable& var);
```

`n_name` というノードの引数を `var` と同じ実体を持つものに置き換えます.  
`var` の中の型 と `addUnivFunc` で渡された `default_args` の対応するものの中の型
が同じである必要があります.  
そうでない場合は失敗し, `false` が返されます.

成功した場合 `true` が返されます.

## `setPriority`

```c++
bool setPriority(const std::string& n_name, int priority);
```

`n_name` と言う名前のノードの実行優先度として `priority` を設定します.  
初期設定は `0` です. 同時に実行できるノードで優先度の高いものから実行されていきます.

成功した場合 `true` が返されます.

## `allocateFunc`

```c++
bool allocateFunc(const std::string& f_name, const std::string& n_name);
```

`n_name` という名前のノードに `f_name` と言う名前の関数を割り当てます.  
`addUnivFunc` で登録された 関数オブジェクトがノードの `func` にコピーされます.  
メンバ変数などの状態を持つ関数オブジェクトである場合, リセットする方法としても使えます.

成功した場合 `true` が返されます.

## `linkNode`

```c++
bool linkNode(const std::string& src_node, std::size_t src_arg,
              const std::string& dst_node, std::size_t dst_arg);
```

`src_node` と言う名前のノードの `src_arg` 番目の引数を
`dst_node` と言う名前のノードの `dst_arg` 番目の引数に対応させます.

`src_node` の`func` が実行された後,
`src_node` と言う名前のノードの `src_arg` 番目の引数の計算結果が
`dst_node` と言う名前のノードの `dst_arg` 番目の引数として渡され
(同じ実体が使われます), `dst_node` の`func` が実行されるようになります.

引数の型が違う, 又はそのリンクを追加するとループが発生する場合, 失敗します.

成功した場合 `true` が返されます.

## `unlinkNode`

```c++
bool unlinkNode(const std::string& dst_node, std::size_t dst_arg);
```

`dst_node` と言う名前のノードの `dst_arg` 番目に入ってくるリンクを破棄します.

成功した場合 `true` が返されます.

## `supposeInput`

```c++
bool supposeInput(std::vector<Variable>& vars);
```

`vars` を用いて, このパイプラインの入力の 個数, 型 を決定します.  
`vars` に渡された `Variable` と同じ実体をさすものが 入力ノードの引数となります.

成功した場合 `true` が返されます.


## `supposeOutput`

```c++
bool supposeOutput(std::vector<Variable>& vars);
```

`vars` を用いて, このパイプラインの出力の 個数, 型 を決定します.  
`vars` に渡された `Variable` と同じ実体をさすものが 入力ノードの引数となります.  
この関数を呼んだ後に 下の `run` を呼びそれが成功した場合,
このパイプラインの計算結果が `vars` 等が指す実体に
コピー(`Variable::copyTo`)されているでしょう.

成功した場合 `true` が返されます.

## `run`

```c++
bool run(Report* preport = nullptr);
```

このパイプラインを実行します.  
`preport` に `Report` インスタンスのポインタを渡した場合,
実行時にかかった 各ノードの計算時間, および
このパイプラインのトータルの計算時間が格納されます.

成功した場合 `true` が返されます.

## `getNodes`

```c++
const std::map<std::string, Node>& getNodes() const noexcept;
```

ノードの名前を key, それに対応するノードのオブジェクトを value とする  
`std::map` を返します.

## `getLinks`

```c++
const std::vector<Link>& getLinks() const noexcept;
```

ノード間のリンクを羅列した配列を返します.

