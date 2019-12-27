
# Variable クラス

`Variable` クラスはFaseにおけるローカル変数を扱うクラスです.

`Variable` は中にあらゆる型を内包する
[`std::any`](https://cpprefjp.github.io/reference/any/any.html)
(`std::shared_ptr<void>`でも可) の性質, 同じ実体を複数で共有する
[`std::shared_ptr`](https://cpprefjp.github.io/reference/memory/shared_ptr.html)
の性質, 空の値を持つことができる
[`std::optional`](https://cpprefjp.github.io/reference/optional/optional.html)
の性質 を持ちます.

気分としては `shared_ptr<any<optional<T>>>` に近いものがあります.  
(当然 `any` は非テンプレートクラスなので, `any<optional<T>>` は便宜上のものです.)

ただし, 中に持つ値の型 (上で言う`T`) は
コピーコンストラクタとコピー代入演算子を持っている必要があります.

## デフォルトコンストラクタ

型として`void`を持ち値を持たない実体を作成し, それを指すインタンスとなります.

## `Variable<T>(std::unique_ptr<T>&& ptr)` コンストラクタ

`ptr` の持つ値を持つ実体を作成し, それを指すインタンスとなります.  
下の`set(ptr)`がよばれたインスタンスが出来ることと同値です.  

## `Variable(const std::type_index& type)` コンストラクタ

`type` の型を持ち値を持たない実体を作成し, それを指すインタンスとなります.

## `Variable<T>(T* ptr)` コンストラクタ

`*ptr` を指し示す所有権を持たない実体を作成し, それを指すインスタンスとなります.  
つまり、このインスタンスが指す実体が消える際に `*ptr` のデストラクタは呼ばれません.  
このインスタンスには、寿命が尽きる、もしくはムーブ代入演算子が呼ばれる際に指している実体の中身を空にする義務が発生します.  
この義務はムーブによってのみ移譲されます.  
このインスタンスよりも先に`*ptr`の寿命が尽きると未定義動作となります.  

```c++
Variable v;
int c = 123;
{
    Variable v2(&c);
    v = v2.ref();
    assert(*v.getReader<int>() == 123);
    *v.getWriter<int>() = 456;
} //  v2 was deleted! And the pointed by v2 and v was cleared.
assert(bool(v) == false);
assert(c == 456);
```

```c++
Variable v;
{
    int a;
    v = Variable(&a);
} //  a was deleted!

*v.getReader<int>() = 0;  // Undefined behavior!!
```

## ムーブコンストラクタ, ムーブ代入演算子

自然なムーブコンストラクタを提供します.

## `Variable::set<T>(std::shared_ptr<T>&& ptr)`

`ptr` を使って実体を作成し, このインスタンスが指し示す実体とします.  
この実体はムーブされた`ptr`を保持し,
この実体を指し示す`Variable`インスタンスがなくなったとき,  
(外に同じ`std::shared_ptr`がなければ)
実体が持つリソースは`ptr`が持つデリータを使って解放されます。

```c++
Variable v;
auto ptr = std::make_shared<int>(1);
v.set({ptr});
assert(v.getReader<int>() == ptr);
```

## `Variable::create<T, Args...>(Args&&... args)`

上の`set(std::make_shared<T>(std::forward<Args...>(args)));`
を呼ぶに等しいです。

## `Variable::ref()`

同じ実体を指す `Variable` のインスタンスを作成します.

```c++
Variable v, v2;
v.create<int>(1);
v2 = v.ref();
assert(v.getReader<int>() == v2.getReader<int>());
```

## `Variable::clone()`, コピーコンストラクタ

複製した実体を持つ `Variable` のインスタンスを作成します.  
元のインスタンスがさす実体と, 複製された実体は独立したものとなります.

```c++
Variable v = std::make_unique<int>(1);

Variable v2 = v.clone();  // equal to "Variable v2 = v;"
*v2.getWriter<int>() = 2;
assert(*v.getReader<int>() == 1);
assert(*v2.getReader<int>() == 2);
```

## コピー代入演算子

複製した実体を作成し 自分の指す実体をそれに置き換えます.

## `Variable::copyTo(Variable& another)`

`another` が持つ実体の値に
自分が持つ実体の値をコピーをします（正確には コピー代入演算子を呼ぶ) .

ただし, どちらかが指す実体がを持たない場合, 処理が特殊になります.
詳細は以下の表を参照してください.

* `a.copyTo(b)`

`A`, `B` はそれぞれ, `void`以外の異なる任意の型を示します.

| `a`の型 | `b`の型 | `a`の値 | `b`の値 | 動作 |
|:---:|:---:|:---:|:---:|:---|
| `A`又は`void` | `void` | * | × | 何も起きない |
| `void` | `B` | × | * | `WrongTypeCast` を例外として投げる |
| `A` | `B` | * | * | `WrongTypeCast` を例外として投げる |
| `A` | `A` | ある | ある | `a`の値を`b`の値にコピー |
| `A` | `A` | ない | ある | `TryToGetEmptyVariable` を例外として投げる |
| `A` | `A` | ない | ない | 何も起きない |
| `A` | `A` | ある | ない | `a`の値を複製し,`b`のさす実体の値とする |

特に `test/test_variable.h` の `"Empry Ref : Asign"`,
`"Empry Ref : Copy"` テストで行われる挙動の違いに注意してください.

## `operator bool()`

このインスタンスが指す実体が値を持っているかを示します。

```c++
Variable v1;
Variable v2 = std::make_unique<int>(0);
Variable v3 = typeid(int);
Variable v4 = v2.ref();

assert(bool(v1) == false);
assert(bool(v2) == true);
assert(bool(v3) == false);
assert(bool(v4) == true);
```

## `Variable::free()`

このインスタンスが指し示す実体の値を解放します.

```c++
Variable v = std::make_unique<int>(1);
Variable v2 = v.ref();

assert(v && v2);

v.free();

assert(bool(v) == false);
assert(bool(v2) == false);
```

## さらに

`test/test_variable.h` を参照してください.
